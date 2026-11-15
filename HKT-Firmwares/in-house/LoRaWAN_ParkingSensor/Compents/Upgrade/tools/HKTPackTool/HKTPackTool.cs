// ============================================================================
// HKTPackTool.cs - standalone PC tool that builds upgrade release packages
// for the LoRaWAN_ParkingSensor BLE diff/full upgrade channel.
//
// Modes
//   GUI (double-click / no args):
//       pick old/new .bin, versions auto-guessed from filename (V<hw>.<sw>)
//       or from the embedded "FTKH" record at image offset 0x200, then build.
//   --diff <old.bin> <new.bin> <out> <hw> <sw> <baseHw> <baseSw>
//   --full <bin> <out> <hw> <sw>
//   --selftest [foreign_patch.bin]
//       CRC32 golden vector, LZO encoder/decoder round-trip battery,
//       bsdiff build/apply battery; optionally decode a patch produced by
//       another minilzo-based tool (cross-validation of the decoder).
//
// Output format (must stay in sync with firmware + Android):
//   diff : 24B "HKTU" release header (little-endian) + payload
//          payload = 48B "DIFF" header + segmented-LZO bsdiff body
//   full : 24B "HKTU" release header + raw image
//   The Android app (UpgradePackage.kt) strips the HKTU header and sends the
//   payload; the device (app_bspatch.c) checks the 48B header, walks the
//   segments and applies the patch. This tool mirrors ALL of those checks:
//   a package is only written after the generated patch is re-applied with a
//   firmware-faithful decoder and reproduces the new image byte-for-byte.
//
// LZO ports are mechanical translations of the repo's minilzo.c
// (lzo1x_1_compress core + lzo1x_decompress_safe, LZO1X flavour).
//
// Build (C#5, .NET Framework 4.x csc - no $"" interpolation, no ?.):
//   build_HKTPackTool.bat
// ============================================================================
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace HktPackTool
{
    internal static class Program
    {
        [DllImport("kernel32.dll")]
        private static extern bool FreeConsole();

        [STAThread]
        private static int Main(string[] args)
        {
            try
            {
                if (args.Length == 0)
                {
                    FreeConsole();                       // launched by double-click: drop the console
                    Application.EnableVisualStyles();
                    Application.SetCompatibleTextRenderingDefault(false);
                    Application.Run(new ToolForm());
                    return 0;
                }
                if (args[0] == "--diff") return CliDiff(args);
                if (args[0] == "--full") return CliFull(args);
                if (args[0] == "--selftest") return SelfTest.Run(Console.Out, args.Length > 1 ? args[1] : null);
                Console.WriteLine("usage: HKTPackTool | --diff <old.bin> <new.bin> <out> <hw> <sw> <baseHw> <baseSw>");
                Console.WriteLine("               | --full <bin> <out> <hw> <sw> | --selftest [foreign_patch.bin]");
                return 2;
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("ERROR: " + ex.Message);
                return 1;
            }
        }

        private static int CliDiff(string[] a)
        {
            if (a.Length != 8) { Console.Error.WriteLine("--diff <old.bin> <new.bin> <out> <hw> <sw> <baseHw> <baseSw>"); return 2; }
            byte[] oldB = File.ReadAllBytes(a[1]);
            byte[] newB = File.ReadAllBytes(a[2]);
            PkgResult r = Package.MakeDiff(oldB, newB, ParseV(a[4]), ParseV(a[5]), ParseV(a[6]), ParseV(a[7]), Console.Out);
            File.WriteAllBytes(a[3], r.File);
            Console.WriteLine("written: " + a[3] + " (" + r.File.Length + " B)");
            return 0;
        }

        private static int CliFull(string[] a)
        {
            if (a.Length != 5) { Console.Error.WriteLine("--full <bin> <out> <hw> <sw>"); return 2; }
            byte[] bin = File.ReadAllBytes(a[1]);
            PkgResult r = Package.MakeFull(bin, ParseV(a[3]), ParseV(a[4]), Console.Out);
            File.WriteAllBytes(a[2], r.File);
            Console.WriteLine("written: " + a[2] + " (" + r.File.Length + " B)");
            return 0;
        }

        private static int ParseV(string s)
        {
            int v = int.Parse(s);
            if (v < 0 || v > 255) throw new Exception("version byte out of range: " + s);
            return v;
        }
    }

    // =========================================================================
    // CRC32-IEEE (reflected 0xEDB88320) - same as firmware upg_crc32 / Android
    // CRC32.ieee / hydrant boot_crc32.
    // =========================================================================
    internal static class Crc32Ieee
    {
        private static readonly uint[] Table = BuildTable();

        private static uint[] BuildTable()
        {
            uint[] t = new uint[256];
            for (uint i = 0; i < 256; i++)
            {
                uint c = i;
                for (int k = 0; k < 8; k++) c = ((c & 1) != 0) ? (c >> 1) ^ 0xEDB88320u : c >> 1;
                t[i] = c;
            }
            return t;
        }

        public static uint Of(byte[] data, int off, int len)
        {
            uint c = 0xFFFFFFFFu;
            for (int i = 0; i < len; i++) c = (c >> 8) ^ Table[(c ^ data[off + i]) & 0xFF];
            return c ^ 0xFFFFFFFFu;
        }

        public static uint Of(byte[] data) { return Of(data, 0, data.Length); }
    }

    // =========================================================================
    // LZO1X-1 ports (from Compents/Upgrade/minilzo/minilzo.c). Both sides are
    // mechanical translations; the decoder additionally bounds-checks like the
    // *_safe variant and throws LzoException instead of returning codes.
    // =========================================================================
    internal class LzoException : Exception
    {
        public LzoException(string m) : base(m) { }
    }

    internal static class Lzo
    {
        private const int M2_MAX_OFFSET = 0x0800;
        private const int M3_MAX_OFFSET = 0x4000;
        private const int M4_MAX_OFFSET = 0xbfff;
        private const int M2_MAX_LEN = 8;
        private const int M3_MAX_LEN = 33;
        private const int M4_MAX_LEN = 9;
        private const int M3_MARKER = 32;
        private const int M4_MARKER = 16;
        private const int D_BITS = 14;
        private const uint D_MASK = (1u << D_BITS) - 1;          // 0x3FFF

        // ---- decoder -------------------------------------------------------
        // Faithful port of lzo1x_decompress_safe (LZO1X, non-COPY_DICT),
        // restructured as a state machine because C# forbids goto into
        // nested blocks. Opcode context (what a t<16 byte means):
        //   TOP     - literal run of t+3 bytes (t == 0: extended length)
        //   POSTLIT - M1: 3-byte match, offset (1+M2_MAX_OFFSET)+(t>>2)+(b<<2)
        //   MATCH   - M1: 2-byte match, offset 1+(t>>2)+(b<<2)
        // output buffer capacity = output.Length; returns produced length.
        public static int Decompress(byte[] input, byte[] output)
        {
            const int ST_TOP = 0, ST_POSTLIT = 1, ST_MATCH = 2;
            int cap = output.Length;
            int ip = 0, op = 0;
            int t;

            if (input.Length < 1) throw new LzoException("input overrun: empty");

            // init: first byte > 17 encodes a leading literal run of (b - 17)
            if (input[0] > 17)
            {
                t = input[ip++] - 17;
                if (t < 4)
                {
                    // C: "goto match_next" - leading run acts as trailing literals
                    NeedOp(cap, op, t); NeedIp(input, ip, t + 3);
                    while (t-- > 0) output[op++] = input[ip++];
                    t = input[ip++];
                    return Run(input, output, cap, ref ip, ref op, t, ST_MATCH);
                }
                NeedOp(cap, op, t); NeedIp(input, ip, t + 3);
                while (t-- > 0) output[op++] = input[ip++];
                t = input[ip++];
                return Run(input, output, cap, ref ip, ref op, t, ST_POSTLIT);
            }
            t = input[ip++];
            return Run(input, output, cap, ref ip, ref op, t, ST_TOP);
        }

        private static int Run(byte[] input, byte[] output, int cap, ref int ip, ref int op, int t, int state)
        {
            const int ST_TOP = 0, ST_POSTLIT = 1, ST_MATCH = 2;
            for (;;)
            {
                int mPos;

                if (state == ST_TOP)
                {
                    if (t < 16)
                    {
                        // literal run of t + 3 bytes
                        if (t == 0)
                        {
                            NeedIp(input, ip, 1);
                            while (input[ip] == 0) { t += 255; ip++; NeedIp(input, ip, 1); }
                            t += 15 + input[ip++];
                        }
                        NeedOp(cap, op, t + 3); NeedIp(input, ip, t + 6);
                        int n = t + 3;
                        while (n-- > 0) output[op++] = input[ip++];
                        t = input[ip++];
                        state = ST_POSTLIT;
                        continue;
                    }
                }
                else if (state == ST_POSTLIT && t < 16)
                {
                    // M1 right after a literal run: length 3
                    mPos = op - (1 + M2_MAX_OFFSET) - (t >> 2) - (input[ip++] << 2);
                    TestLb(mPos);
                    NeedOp(cap, op, 3);
                    output[op++] = output[mPos++];
                    output[op++] = output[mPos++];
                    output[op++] = output[mPos];
                    t = input[ip - 2] & 3;                  // match_done
                    if (t != 0) goto trailing;
                    t = input[ip++];
                    state = ST_TOP;
                    continue;
                }

                // ---- match dispatch (t >= 16, or t < 16 in MATCH context) ----
                if (t >= 64)
                {
                    // short-offset family: offset 1 + ((t>>2)&7) + (b<<3), copy len (t>>5)+1
                    mPos = op - 1 - ((t >> 2) & 7) - (input[ip++] << 3);
                    t = (t >> 5) - 1;
                    TestLb(mPos);
                    NeedOp(cap, op, t + 2);
                }
                else if (t >= 32)
                {
                    // M3: offset 1 + (LE16>>2), copy len 2 + t
                    t &= 31;
                    if (t == 0)
                    {
                        NeedIp(input, ip, 1);
                        while (input[ip] == 0) { t += 255; ip++; NeedIp(input, ip, 1); }
                        t += 31 + input[ip++];
                        NeedIp(input, ip, 2);
                    }
                    mPos = op - 1 - (int)(((uint)input[ip] | ((uint)input[ip + 1] << 8)) >> 2);
                    ip += 2;
                    TestLb(mPos);
                    if (t <= 0) throw new LzoException("corrupted: zero match length");
                    NeedOp(cap, op, t + 2);
                }
                else if (t >= 16)
                {
                    // M2/M4 family: optional 0x4000 bias; EOF marker lives here
                    mPos = op - ((t & 8) << 11);
                    t &= 7;
                    if (t == 0)
                    {
                        NeedIp(input, ip, 1);
                        while (input[ip] == 0) { t += 255; ip++; NeedIp(input, ip, 1); }
                        t += 7 + input[ip++];
                        NeedIp(input, ip, 2);
                    }
                    mPos -= (int)(((uint)input[ip] | ((uint)input[ip + 1] << 8)) >> 2);
                    ip += 2;
                    if (mPos == op)
                    {
                        // EOF marker: input must be fully consumed here
                        if (ip != input.Length) throw new LzoException("input not consumed (" + ip + "/" + input.Length + ")");
                        return op;
                    }
                    mPos -= 0x4000;
                    TestLb(mPos);
                    if (t <= 0) throw new LzoException("corrupted: zero match length");
                    NeedOp(cap, op, t + 2);
                }
                else
                {
                    // M1 in match context: length 2, offset 1 + (t>>2) + (b<<2)
                    mPos = op - 1 - (t >> 2) - (input[ip++] << 2);
                    TestLb(mPos);
                    NeedOp(cap, op, 2);
                    output[op++] = output[mPos++];
                    output[op++] = output[mPos];
                    t = input[ip - 2] & 3;                  // match_done
                    if (t != 0) goto trailing;
                    t = input[ip++];
                    state = ST_TOP;
                    continue;
                }

                // generic match copy: 2 + t bytes, byte-forward (overlap-safe)
                output[op++] = output[mPos++];
                output[op++] = output[mPos++];
                do { output[op++] = output[mPos++]; } while (--t > 0);

                t = input[ip - 2] & 3;                      // trailing-literal count
                if (t == 0) { t = input[ip++]; state = ST_TOP; continue; }

            trailing:
                NeedOp(cap, op, t); NeedIp(input, ip, t + 3);
                output[op++] = input[ip++];
                if (t > 1) { output[op++] = input[ip++]; if (t > 2) { output[op++] = input[ip++]; } }
                t = input[ip++];
                state = ST_MATCH;
            }
        }

        private static void NeedIp(byte[] input, int p, int n)
        {
            if (p + n > input.Length) throw new LzoException("input overrun @" + p);
        }

        private static void NeedOp(int cap, int p, int n)
        {
            if (p + n > cap) throw new LzoException("output overrun @" + p);
        }

        private static void TestLb(int p)
        {
            if (p < 0) throw new LzoException("lookbehind overrun");
        }

        // ---- encoder -------------------------------------------------------
        // Faithful port of lzo1x_1_compress (do_compress core, DETERMINISTIC
        // variant: 49152-byte blocks, dict reset per block).
        public static byte[] Compress(byte[] input)
        {
            int inLen = input.Length;
            var op = new List<byte>(inLen + inLen / 16 + 67);
            int t = 0;
            int ip = 0;
            int l = inLen;
            ushort[] dict = new ushort[1 << D_BITS];

            while (l > 20)
            {
                int ll = l > 49152 ? 49152 : l;
                t = DoCompress(input, ip, ll, op, t, dict);
                ip += ll;
                l -= ll;
            }
            t += l;

            if (t > 0)
            {
                int ii = inLen - t;
                if (op.Count == 0 && t <= 238) op.Add((byte)(17 + t));
                else if (t <= 3) { int ix = op.Count - 2; op[ix] = (byte)(op[ix] | t); }
                else if (t <= 18) op.Add((byte)(t - 3));
                else
                {
                    int tt = t - 18;
                    op.Add(0);
                    while (tt > 255) { tt -= 255; op.Add(0); }
                    op.Add((byte)tt);
                }
                for (int k = 0; k < t; k++) op.Add(input[ii + k]);
            }

            op.Add((byte)(M4_MARKER | 1));                  // EOF marker
            op.Add(0);
            op.Add(0);
            return op.ToArray();
        }

        private static int DoCompress(byte[] input, int inStart, int inLen, List<byte> op, int ti, ushort[] dict)
        {
            int inEnd = inStart + inLen;
            int ipEnd = inStart + inLen - 20;
            int ii = inStart;
            int ip = inStart + (ti < 4 ? 4 - ti : 0);

            for (;;)
            {
                if (ip >= ipEnd) break;
                uint dindex = DIndex1(input, ip);
                int mPos = 0, mOff = 0;
                bool have = Lookup(dict, dindex, ip, inStart, ref mPos, ref mOff);
                bool gate = have && (mOff <= M2_MAX_OFFSET || input[mPos + 3] == input[ip + 3]);
                if (!gate)
                {
                    // second probe with the alternate hash index
                    dindex = DIndex2(dindex);
                    have = Lookup(dict, dindex, ip, inStart, ref mPos, ref mOff);
                    gate = have && (mOff <= M2_MAX_OFFSET || input[mPos + 3] == input[ip + 3]);
                }
                // try_match: 4-byte compare on the gated candidate
                bool matched = gate &&
                    input[mPos] == input[ip] && input[mPos + 1] == input[ip + 1] &&
                    input[mPos + 2] == input[ip + 2] && input[mPos + 3] == input[ip + 3];
                if (!matched)
                {
                    dict[dindex] = (ushort)(ip - inStart);
                    ip += 1 + ((ip - ii) >> 5);
                    continue;
                }
                dict[dindex] = (ushort)(ip - inStart);

                ii -= ti; ti = 0;
                int t = ip - ii;
                if (t != 0)
                {
                    if (t <= 3)
                    {
                        int ix = op.Count - 2;
                        op[ix] = (byte)(op[ix] | t);
                        for (int k = 0; k < t; k++) op.Add(input[ii + k]);
                    }
                    else
                    {
                        if (t <= 18) op.Add((byte)(t - 3));
                        else
                        {
                            int tt = t - 18;
                            op.Add(0);
                            while (tt > 255) { tt -= 255; op.Add(0); }
                            op.Add((byte)tt);
                        }
                        for (int k = 0; k < t; k++) op.Add(input[ii + k]);
                    }
                }

                int mLen = 4;
                while (ip + mLen < inEnd && input[ip + mLen] == input[mPos + mLen]) mLen++;
                mOff = ip - mPos;
                ip += mLen;
                ii = ip;

                if (mLen <= M2_MAX_LEN && mOff <= M2_MAX_OFFSET)
                {
                    mOff -= 1;
                    op.Add((byte)(((mLen - 1) << 5) | ((mOff & 7) << 2)));
                    op.Add((byte)(mOff >> 3));
                }
                else if (mOff <= M3_MAX_OFFSET)
                {
                    mOff -= 1;
                    if (mLen <= M3_MAX_LEN) op.Add((byte)(M3_MARKER | (mLen - 2)));
                    else
                    {
                        mLen -= M3_MAX_LEN;
                        op.Add((byte)M3_MARKER);
                        while (mLen > 255) { mLen -= 255; op.Add(0); }
                        op.Add((byte)mLen);
                    }
                    op.Add((byte)(mOff << 2));
                    op.Add((byte)((mOff >> 6) & 0xFF));
                }
                else
                {
                    mOff -= 0x4000;
                    if (mLen <= M4_MAX_LEN) op.Add((byte)(M4_MARKER | ((mOff >> 11) & 8) | (mLen - 2)));
                    else
                    {
                        mLen -= M4_MAX_LEN;
                        op.Add((byte)(M4_MARKER | ((mOff >> 11) & 8)));
                        while (mLen > 255) { mLen -= 255; op.Add(0); }
                        op.Add((byte)mLen);
                    }
                    op.Add((byte)(mOff << 2));
                    op.Add((byte)((mOff >> 6) & 0xFF));
                }
            }
            return inEnd - (ii - ti);
        }

        // GINDEX + LZO_CHECK_MPOS_NON_DET (dict-use-ptr = 0 variant)
        private static bool Lookup(ushort[] dict, uint dindex, int ip, int inStart, ref int mPos, ref int mOff)
        {
            int entry = dict[dindex];
            int ipOff = ip - inStart;
            if (ipOff <= entry) return false;
            int off = ipOff - entry;
            if (off == 0 || off > M4_MAX_OFFSET) return false;
            mOff = off;
            mPos = ip - off;
            return true;
        }

        // D_INDEX1 / D_INDEX2 / DX3 (LZO_HASH_LZO_INCREMENTAL_A, D_BITS = 14)
        private static uint DIndex1(byte[] b, int p)
        {
            uint v = ((((((uint)b[p + 3] << 6) ^ b[p + 2]) << 5) ^ b[p + 1]) << 5) ^ b[p];
            return ((0x21u * v) >> 5) & D_MASK;
        }

        private static uint DIndex2(uint d)
        {
            return (d & (D_MASK & 0x7ffu)) ^ (((D_MASK >> 1) + 1) | 0x1fu);
        }
    }

    // =========================================================================
    // bsdiff emitter + firmware-faithful applier.
    //
    // The emitter is a greedy hash-chain matcher over `old` producing classic
    // bsdiff (addLen, copyLen, seek) triples: matched regions become diff runs
    // (which are byte-differences and therefore mostly zeros - LZO squashes
    // them), unmatched regions become extra literals. Every triple keeps
    // oldpos+len within `old`, so the firmware always takes its fast path.
    //
    // ApplyPatch mirrors app_bspatch.c exactly (header checks, segment walk,
    // ctrl bounds, fast/slow add path, extra copy) - it is the self-check the
    // generated package must pass before the tool writes it out.
    // =========================================================================
    internal static class Bsdiff
    {
        // Mirror of the firmware offtin: value = buf[7]·256^7 + ... + buf[0],
        // sign in buf[7] bit 7. For the magnitudes this tool emits (< 2^56)
        // that is 7 little-endian value bytes + a sign byte.
        public static void Offtout(long x, byte[] buf, int off)
        {
            long y = x < 0 ? -x : x;
            buf[off + 7] = (byte)(((y >> 56) & 0x7F) | (x < 0 ? 0x80L : 0L));
            for (int i = 0; i <= 6; i++) buf[off + i] = (byte)((y >> (8 * i)) & 0xFF);
        }

        public static long Offtin(byte[] b, int off)
        {
            long y = b[off + 7] & 0x7F;
            y = y * 256 + b[off + 6];
            y = y * 256 + b[off + 5];
            y = y * 256 + b[off + 4];
            y = y * 256 + b[off + 3];
            y = y * 256 + b[off + 2];
            y = y * 256 + b[off + 1];
            y = y * 256 + b[off + 0];
            return (b[off + 7] & 0x80) != 0 ? -y : y;
        }

        // Builds the logical ctrl/diff/extra byte stream (pre-segmentation).
        public static byte[] BuildStream(byte[] oldB, byte[] newB)
        {
            const int HASH_BITS = 16;
            const int HASH_SIZE = 1 << HASH_BITS;
            const int MIN_MATCH = 16;
            const int MAX_CHAIN = 24;

            // hash chains over `old` (8-byte key)
            int[] head = new int[HASH_SIZE];
            for (int i = 0; i < HASH_SIZE; i++) head[i] = -1;
            int[] prev = new int[oldB.Length];
            for (int i = 0; i < oldB.Length; i++) prev[i] = -1;
            for (int i = 0; i + 8 <= oldB.Length; i++)
            {
                int h = HashKey(oldB, i);
                prev[i] = head[h];
                head[h] = i;
            }

            var stream = new List<byte>(newB.Length + 4096);
            byte[] ctrl = new byte[24];
            int j = 0, litStart = 0;
            long oldPos = 0;

            while (j < newB.Length)
            {
                int best = 0, bestPos = -1;
                if (j + MIN_MATCH <= newB.Length)
                {
                    int h = HashKey(newB, j);
                    int cand = head[h];
                    for (int tries = 0; cand >= 0 && tries < MAX_CHAIN; cand = prev[cand], tries++)
                    {
                        if (!Eq8(newB, j, oldB, cand)) continue;
                        int len = 8;
                        while (j + len < newB.Length && cand + len < oldB.Length &&
                               newB[j + len] == oldB[cand + len]) len++;
                        if (len > best) { best = len; bestPos = cand; if (best > 8192) break; }
                    }
                }

                if (best >= MIN_MATCH && bestPos >= 0)
                {
                    // bestPos + best <= oldB.Length holds by the extension bound.
                    // bsdiff triple order is add THEN seek, so the seek must ride
                    // on an add-less triple of its own before the match run.
                    int lit = j - litStart;
                    EmitTriple(stream, ctrl, 0, lit, bestPos - oldPos);
                    for (int k = 0; k < lit; k++) stream.Add(newB[litStart + k]);
                    oldPos = bestPos;
                    EmitTriple(stream, ctrl, best, 0, 0);
                    for (int k = 0; k < best; k++)
                        stream.Add((byte)(newB[j + k] - oldB[bestPos + k]));   // zeros for a true match
                    oldPos += best;
                    j += best;
                    litStart = j;
                }
                else
                {
                    j++;
                }
            }

            int tail = newB.Length - litStart;
            if (tail > 0 || stream.Count == 0)
            {
                EmitTriple(stream, ctrl, 0, tail, 0);
                for (int k = 0; k < tail; k++) stream.Add(newB[litStart + k]);
            }
            return stream.ToArray();
        }

        private static void EmitTriple(List<byte> stream, byte[] ctrl, long add, long copy, long seek)
        {
            Offtout(add, ctrl, 0);
            Offtout(copy, ctrl, 8);
            Offtout(seek, ctrl, 16);
            for (int i = 0; i < 24; i++) stream.Add(ctrl[i]);
            // NOTE: bsdiff stream order is ctrl triple, then addLen diff bytes,
            // then copyLen extra bytes - callers append the payloads right after.
        }

        private static int HashKey(byte[] b, int p)
        {
            uint a = (uint)(b[p] | (b[p + 1] << 8) | (b[p + 2] << 16) | (b[p + 3] << 24));
            uint h = a * 2654435761u;
            h ^= h >> 15;
            return (int)(h & 0xFFFF);
        }

        private static bool Eq8(byte[] a, int pa, byte[] b, int pb)
        {
            for (int k = 0; k < 8; k++) if (a[pa + k] != b[pb + k]) return false;
            return true;
        }

        // ---- firmware-faithful apply (self-check) ---------------------------
        // rawPatch = 48B DIFF header + segmented body, exactly what the device
        // receives. Throws on any firmware-side rejection.
        public static byte[] ApplyPatch(byte[] oldB, byte[] rawPatch)
        {
            PkgHeader h = Package.ParseDiffHeader(rawPatch);
            // segment walk -> logical stream
            byte[] logical = ReadSegments(rawPatch, h, oldB.Length);
            // bspatch walk
            var sc = new StreamCursor(logical);
            var result = new byte[h.NewFwSize];
            long newpos = 0, oldpos = 0;
            byte[] buf = new byte[24];
            while (newpos < h.NewFwSize)
            {
                if (sc.Read(buf, 0, 24) != 0) throw new Exception("apply: ctrl read fail @" + newpos);
                long add = Offtin(buf, 0), copy = Offtin(buf, 8), seek = Offtin(buf, 16);
                if (add < 0 || add > int.MaxValue || copy < 0 || copy > int.MaxValue ||
                    newpos + add > h.NewFwSize) throw new Exception("apply: bad ctrl @" + newpos);
                // diff string (fast path when the run stays inside old; slow path
                // mirrors the firmware clamp at oldsize)
                for (long i = 0; i < add; i++)
                {
                    int d = sc.ReadByte();
                    if (oldpos + i < oldB.Length) d += oldB[oldpos + i];
                    result[newpos + i] = (byte)d;
                }
                newpos += add;
                oldpos += add;
                if (newpos + copy > h.NewFwSize) throw new Exception("apply: extra overrun @" + newpos);
                if (sc.Read(result, (int)newpos, (int)copy) != 0) throw new Exception("apply: extra read fail @" + newpos);
                newpos += copy;
                oldpos += seek;
            }
            if (sc.HasMore) throw new Exception("apply: patch stream not fully consumed");
            if (Crc32Ieee.Of(result) != h.NewFwCrc) throw new Exception("apply: new_fw_crc mismatch");
            return result;
        }

        // Segment layer walk (lzo_unpack + reader in app_bspatch.c).
        public static byte[] ReadSegments(byte[] rawPatch, PkgHeader h, int unusedOldLen)
        {
            const int DIFF_HDR = 48;
            const int SEG_MAX = 4419;
            var logical = new List<byte>((int)h.PatchSize);
            int pos = DIFF_HDR;
            int end = DIFF_HDR + (int)h.PatchSize;
            while (pos < end)
            {
                if (pos + 6 > end) throw new Exception("segment header truncated @" + pos);
                uint len = U32(rawPatch, pos);
                int compress = rawPatch[pos + 4];
                int cs = rawPatch[pos + 5];
                pos += 6;
                if (len == 0 || len > SEG_MAX) throw new Exception("segment len bad " + len);
                if (pos + len > end) throw new Exception("segment data truncated @" + pos);
                byte[] outBuf;
                int outLen;
                if (compress == 0)
                {
                    if (len > h.Level) throw new Exception("raw segment " + len + " > level " + h.Level);
                    outBuf = new byte[len];
                    Array.Copy(rawPatch, pos, outBuf, 0, (int)len);
                    outLen = (int)len;
                }
                else if (compress == 1)
                {
                    byte[] comp = new byte[len];
                    Array.Copy(rawPatch, pos, comp, 0, (int)len);
                    outBuf = new byte[h.Level];
                    outLen = Lzo.Decompress(comp, outBuf);
                }
                else throw new Exception("bad compress type " + compress);
                if (CheckSum(outBuf, 0, outLen) != cs) throw new Exception("segment cs fail @" + pos);
                for (int i = 0; i < outLen; i++) logical.Add(outBuf[i]);
                pos += (int)len;
            }
            return logical.ToArray();
        }

        public static byte CheckSum(byte[] d, int off, int len)
        {
            int s = 0;
            for (int i = 0; i < len; i++) s += d[off + i];
            return (byte)(s & 0xFF);
        }

        internal static uint U32(byte[] b, int off)
        {
            return (uint)b[off] | ((uint)b[off + 1] << 8) | ((uint)b[off + 2] << 16) | ((uint)b[off + 3] << 24);
        }
    }

    internal sealed class StreamCursor
    {
        private readonly byte[] b;
        private int p;
        public StreamCursor(byte[] data) { b = data; p = 0; }
        public bool HasMore { get { return p < b.Length; } }
        public int Read(byte[] dst, int off, int len)
        {
            if (p + len > b.Length) return -1;
            Array.Copy(b, p, dst, off, len);
            p += len;
            return 0;
        }
        public int ReadByte()
        {
            if (p >= b.Length) throw new Exception("patch stream exhausted");
            return b[p++];
        }
    }

    // =========================================================================
    // Package layer: 48B DIFF header + segmented-LZO body + 24B HKTU wrapper,
    // full-image mode, and the Android-side (UpgradePackage.kt) adopt mirror.
    // =========================================================================
    internal sealed class PkgHeader
    {
        public uint PatchSize, PatchCrc, Level, NewFwVersion, NewFwSize, NewFwCrc;
        public uint OldFwVersion, OldFwSize, OldFwCrc;
        public int Pid, Cid;
    }

    internal sealed class PkgResult
    {
        public byte[] File;
        public string Info;
    }

    internal static class Package
    {
        public const int HKTU_HDR = 24;
        public const int DIFF_HDR = 48;
        public const uint PID = 0x0007;
        public const uint CID = 0xFFFF;
        public const uint LEVEL = 4096;
        public const int APP_MAX = 0x2C000;          // 176KB image cap
        public const int PATCH_AREA = 0x3F000;       // ext-flash Patch area minus session sector
        public const int SEG_MAX = 4419;             // UPG_LZO_SEG_MAX

        public static PkgResult MakeDiff(byte[] oldB, byte[] newB, int hw, int sw, int baseHw, int baseSw, TextWriter log)
        {
            log.WriteLine("---- 差分包 ------------------------------------------------");
            ImageDefenses(newB, "目标镜像");
            if (oldB.Length > APP_MAX) throw new Exception("底包镜像 " + oldB.Length + " B > APP 上限 " + APP_MAX);

            uint oldCrc = Crc32Ieee.Of(oldB);
            uint newCrc = Crc32Ieee.Of(newB);
            log.WriteLine(string.Format("底包  : v{0}.{1}  {2} B  CRC32={3:X8}", baseHw, baseSw, oldB.Length, oldCrc));
            log.WriteLine(string.Format("目标  : v{0}.{1}  {2} B  CRC32={3:X8}", hw, sw, newB.Length, newCrc));
            if (oldCrc == newCrc) log.WriteLine("警告  : 两份镜像完全相同（差分无意义但仍可生成）");
            if (hw == baseHw && sw == baseSw) log.WriteLine("警告  : 目标版本与底包版本相同，设备侧 new/old 版本检查仍会通过但等于重刷");
            log.WriteLine("注意  : 底包必须是设备正在运行的镜像（设备会按 old_fw_size 对运行镜像算 CRC 比对 old_fw_crc）");
            log.WriteLine("注意  : 底包版本必须等于设备当前运行版本 v" + baseHw + "." + baseSw + "，否则设备拒绝合成");

            // 1. logical bsdiff stream
            byte[] logical = Bsdiff.BuildStream(oldB, newB);
            log.WriteLine(string.Format("bsdiff: 逻辑流 {0} B", logical.Length));

            // 2. segment layer: 4096B chunks, LZO with round-trip gate, raw fallback
            List<byte> body = new List<byte>();
            int nSeg = 0, nLzo = 0, nRaw = 0, storedBytes = 0;
            for (int off = 0; off < logical.Length; off += (int)LEVEL)
            {
                int n = Math.Min((int)LEVEL, logical.Length - off);
                byte[] chunk = new byte[n];
                Array.Copy(logical, off, chunk, 0, n);
                byte cs = Bsdiff.CheckSum(chunk, 0, n);
                byte[] comp = Lzo.Compress(chunk);
                bool use = comp.Length < n && comp.Length <= SEG_MAX;
                if (use)
                {
                    byte[] back = new byte[n];
                    int got;
                    try { got = Lzo.Decompress(comp, back); }
                    catch (LzoException) { got = -1; }
                    if (got != n) use = false;
                    else for (int k = 0; k < n; k++) if (back[k] != chunk[k]) { use = false; break; }
                }
                AddU32(body, use ? (uint)comp.Length : (uint)n);
                body.Add(use ? (byte)1 : (byte)0);
                body.Add(cs);
                if (use) { AddBytes(body, comp); nLzo++; }
                else { AddBytes(body, chunk); nRaw++; }
                storedBytes += (use ? comp.Length : n) + 6;
                nSeg++;
            }
            if (nRaw > 0) log.WriteLine(string.Format("警告  : {0}/{1} 段未压缩存储（回环校验未通过或压缩无收益）", nRaw, nSeg));
            log.WriteLine(string.Format("段层  : {0} 段 (LZO {1} / raw {2})，body {3} B，压缩率 {4:F1}%",
                nSeg, nLzo, nRaw, body.Count, 100.0 * body.Count / logical.Length));

            // 3. 48B DIFF header
            uint patchCrc = Crc32Ieee.Of(body.ToArray(), 0, body.Count);
            var rawPatch = new List<byte>(DIFF_HDR + body.Count);
            AddMagic(rawPatch, "DIFF");
            AddU32(rawPatch, (uint)body.Count);
            AddU32(rawPatch, patchCrc);
            AddU32(rawPatch, LEVEL);
            AddU16(rawPatch, PID);
            AddU16(rawPatch, CID);
            AddU32(rawPatch, (uint)((hw << 8) | sw));
            AddU32(rawPatch, (uint)newB.Length);
            AddU32(rawPatch, newCrc);
            AddU32(rawPatch, (uint)((baseHw << 8) | baseSw));
            AddU32(rawPatch, (uint)oldB.Length);
            AddU32(rawPatch, oldCrc);
            AddU32(rawPatch, 0);
            rawPatch.AddRange(body);
            byte[] patch = rawPatch.ToArray();
            if (patch.Length > PATCH_AREA)
                throw new Exception(string.Format("补丁 {0} B > 设备 Patch 区 {1} B", patch.Length, PATCH_AREA));

            // 4. mandatory self-check: re-apply with the firmware-faithful decoder
            byte[] applied = Bsdiff.ApplyPatch(oldB, patch);
            for (int i = 0; i < newB.Length; i++)
                if (applied[i] != newB[i]) throw new Exception("自检失败: 合成镜像与目标镜像在第 " + i + " 字节不一致");
            log.WriteLine("自检  : 合成回放与目标镜像逐字节一致 PASS");

            // 5. HKTU wrapper + Android adopt mirror
            uint payloadCrc = Crc32Ieee.Of(patch);
            byte[] file = WrapHktu(1, hw, sw, baseSw, oldCrc, patch, payloadCrc);
            AdoptMirror(file, 1);
            log.WriteLine("收包  : Android UpgradePackage 防线镜像校验 PASS");

            var info = string.Format(
                "差分包 v{0}.{1}（底包 v{2}.{3}）\r\n" +
                "载荷 {4} B（0xF0 申报 CRC32={5:X8}）\r\n" +
                "目标镜像 {6} B CRC32={7:X8}\r\n" +
                "底包镜像 {8} B CRC32={9:X8}\r\n" +
                "pid=0x{10:X4} cid=0x{11:X4} level={12}",
                hw, sw, baseHw, baseSw, patch.Length, payloadCrc,
                newB.Length, newCrc, oldB.Length, oldCrc, PID, CID, LEVEL);
            log.WriteLine(info);
            return new PkgResult { File = file, Info = info };
        }

        public static PkgResult MakeFull(byte[] bin, int hw, int sw, TextWriter log)
        {
            log.WriteLine("---- 全量包 ------------------------------------------------");
            ImageDefenses(bin, "镜像");
            uint crc = Crc32Ieee.Of(bin);
            int[] ftkh = ParseFtkh(bin);
            if (ftkh != null)
            {
                if (ftkh[0] != hw || ftkh[1] != sw)
                    throw new Exception(string.Format("版本冲突: 内嵌 FTKH 记录 v{0}.{1} ≠ 填写版本 v{2}.{3}（Android 会按 FW_INFO_MISMATCH 拒收）", ftkh[0], ftkh[1], hw, sw));
                log.WriteLine(string.Format("FTKH  : 内嵌记录 v{0}.{1}（版本唯一真源，已与填写值核对）", ftkh[0], ftkh[1]));
            }
            else log.WriteLine("FTKH  : 镜像无内嵌记录（偏移 0x200 无 FTKH），版本以工具填写为准");

            uint payloadCrc = crc;
            byte[] file = WrapHktu(0, hw, sw, -1, 0, bin, payloadCrc);
            AdoptMirror(file, 0);
            log.WriteLine("收包  : Android UpgradePackage 防线镜像校验 PASS");

            var info = string.Format(
                "全量包 v{0}.{1}\r\n载荷 {2} B（0xF0 申报 CRC32={3:X8}）",
                hw, sw, bin.Length, payloadCrc);
            log.WriteLine(info);
            return new PkgResult { File = file, Info = info };
        }

        // full-image defenses mirrored from UpgradePackage.kt (and the BL
        // install checks): size window, stack top in SRAM, reset vector in APP.
        private static void ImageDefenses(byte[] img, string what)
        {
            if (img.Length < 8192 || img.Length > 180224)
                throw new Exception(string.Format("{0} 大小 {1} B 超出全量镜像合法域 8192..180224", what, img.Length));
            uint sp = Bsdiff.U32(img, 0);
            if ((sp & 0xFFFE0000u) != 0x20000000u)
                throw new Exception(string.Format("{0} 栈顶 0x{1:X8} 不在 SRAM（0x2000xxxx）", what, sp));
            uint rst = Bsdiff.U32(img, 4) & 0xFFFFFFFEu;
            if (rst < 0x08004000u || rst >= 0x08030000u)
                throw new Exception(string.Format("{0} 复位向量 0x{1:X8} 不在 APP 区 0x08004000-0x0802FFFF", what, rst));
        }

        public static int[] ParseFtkh(byte[] img)
        {
            const int FW_INFO_OFFSET = 0x200;
            if (img.Length < FW_INFO_OFFSET + 12) return null;
            if (!(img[FW_INFO_OFFSET] == 'F' && img[FW_INFO_OFFSET + 1] == 'T' &&
                  img[FW_INFO_OFFSET + 2] == 'K' && img[FW_INFO_OFFSET + 3] == 'H')) return null;
            return new[] { img[FW_INFO_OFFSET + 4] | (img[FW_INFO_OFFSET + 5] << 8),
                           img[FW_INFO_OFFSET + 6] | (img[FW_INFO_OFFSET + 7] << 8) };
        }

        // "V<hw>.<sw>" from a firmware filename, e.g. MPS100_AS923_V11.28_x.bin
        public static int[] GuessFromFilename(string path)
        {
            Match m = Regex.Match(Path.GetFileName(path), @"[Vv](\d{1,3})\.(\d{1,3})");
            if (!m.Success) return null;
            int hw = int.Parse(m.Groups[1].Value), sw = int.Parse(m.Groups[2].Value);
            if (hw > 255 || sw > 255) return null;
            return new[] { hw, sw };
        }

        private static byte[] WrapHktu(int type, int hw, int sw, int baseSw, uint baseCrc, byte[] payload, uint payloadCrc)
        {
            byte[] h = new byte[HKTU_HDR];
            h[0] = 0x48; h[1] = 0x4B; h[2] = 0x54; h[3] = 0x55;      // "HKTU"
            h[4] = 1;
            h[5] = (byte)type;
            h[6] = (byte)hw; h[7] = (byte)(hw >> 8);
            h[8] = (byte)sw; h[9] = (byte)(sw >> 8);
            h[10] = baseSw < 0 ? (byte)0xFF : (byte)baseSw;
            h[11] = baseSw < 0 ? (byte)0xFF : (byte)(baseSw >> 8);
            PutU32(h, 12, baseCrc);
            PutU32(h, 16, payloadCrc);
            PutU32(h, 20, (uint)payload.Length);
            byte[] file = new byte[HKTU_HDR + payload.Length];
            Array.Copy(h, file, HKTU_HDR);
            Array.Copy(payload, 0, file, HKTU_HDR, payload.Length);
            return file;
        }

        // Mirror of UpgradePackage.adopt for the two paths this tool emits.
        private static void AdoptMirror(byte[] file, int expectType)
        {
            if (file.Length < HKTU_HDR + 1) throw new Exception("adopt: 文件过小");
            if (!(file[0] == 0x48 && file[1] == 0x4B && file[2] == 0x54 && file[3] == 0x55))
                throw new Exception("adopt: HKTU magic 不符");
            if (file[4] != 1) throw new Exception("adopt: 头版本 != 1");
            int type = file[5];
            if (type != expectType) throw new Exception("adopt: 包类型不符");
            byte[] payload = new byte[file.Length - HKTU_HDR];
            Array.Copy(file, HKTU_HDR, payload, 0, payload.Length);
            if (Crc32Ieee.Of(payload) != Bsdiff.U32(file, 16)) throw new Exception("adopt: 载荷 CRC 不符");
            if ((int)Bsdiff.U32(file, 20) != payload.Length) throw new Exception("adopt: 载荷长度不符");
            if (type == 1 && payload.Length > PATCH_AREA) throw new Exception("adopt: 差分载荷超设备 Patch 区");
            if (type == 1 && (payload.Length == 0 || payload.Length > 262144)) throw new Exception("adopt: 差分载荷超 Android 上限");
            if (type == 0)
            {
                ImageDefenses(payload, "包内镜像");
                int[] fi = ParseFtkh(payload);
                int hw = file[6] | (file[7] << 8), sw = file[8] | (file[9] << 8);
                if (fi != null && (fi[0] != hw || fi[1] != sw)) throw new Exception("adopt: 包头版本与 FTKH 内嵌记录不一致");
            }
        }

        public static PkgHeader ParseDiffHeader(byte[] rawPatch)
        {
            if (rawPatch.Length < DIFF_HDR + 1) throw new Exception("patch too small");
            if (!(rawPatch[0] == 'D' && rawPatch[1] == 'I' && rawPatch[2] == 'F' && rawPatch[3] == 'F'))
                throw new Exception("bad DIFF magic");
            PkgHeader h = new PkgHeader();
            h.PatchSize = Bsdiff.U32(rawPatch, 4);
            h.PatchCrc = Bsdiff.U32(rawPatch, 8);
            h.Level = Bsdiff.U32(rawPatch, 12);
            h.Pid = rawPatch[16] | (rawPatch[17] << 8);
            h.Cid = rawPatch[18] | (rawPatch[19] << 8);
            h.NewFwVersion = Bsdiff.U32(rawPatch, 20);
            h.NewFwSize = Bsdiff.U32(rawPatch, 24);
            h.NewFwCrc = Bsdiff.U32(rawPatch, 28);
            h.OldFwVersion = Bsdiff.U32(rawPatch, 32);
            h.OldFwSize = Bsdiff.U32(rawPatch, 36);
            h.OldFwCrc = Bsdiff.U32(rawPatch, 40);
            if (h.PatchSize != (uint)(rawPatch.Length - DIFF_HDR)) throw new Exception("patch_size mismatch");
            if (h.Level < 256 || h.Level > 4096) throw new Exception("level " + h.Level + " out of 256..4096");
            if (h.Pid != (int)PID) throw new Exception("pid 0x" + h.Pid.ToString("X4") + " != 0x0007");
            if (h.Cid != (int)CID) throw new Exception("cid mismatch");
            if (h.NewFwSize > APP_MAX) throw new Exception("new_fw_size > APP cap");
            byte[] body = new byte[h.PatchSize];
            Array.Copy(rawPatch, DIFF_HDR, body, 0, (int)h.PatchSize);
            if (Crc32Ieee.Of(body) != h.PatchCrc) throw new Exception("patch_crc mismatch");
            return h;
        }

        private static void AddU16(List<byte> l, uint v) { l.Add((byte)v); l.Add((byte)(v >> 8)); }
        private static void AddU32(List<byte> l, uint v)
        {
            l.Add((byte)v); l.Add((byte)(v >> 8)); l.Add((byte)(v >> 16)); l.Add((byte)(v >> 24));
        }
        private static void AddMagic(List<byte> l, string s) { foreach (char c in s) l.Add((byte)c); }
        private static void AddBytes(List<byte> l, byte[] d) { foreach (byte b in d) l.Add(b); }
        private static void PutU32(byte[] b, int off, uint v)
        {
            b[off] = (byte)v; b[off + 1] = (byte)(v >> 8); b[off + 2] = (byte)(v >> 16); b[off + 3] = (byte)(v >> 24);
        }
    }

    // =========================================================================
    // Self-test battery. Everything here must pass before the tool is usable.
    // =========================================================================
    internal static class SelfTest
    {
        public static int Run(TextWriter log, string foreignPatch)
        {
            int pass = 0, fail = 0;
            Action<string, bool> check = (name, ok) =>
            {
                if (ok) { pass++; log.WriteLine("  PASS  " + name); }
                else { fail++; log.WriteLine("  FAIL  " + name); }
            };

            log.WriteLine("== CRC32 golden ==");
            byte[] g = { 1, 2, 3, 4, 5, 6, 7, 8 };
            check("crc32(01..08)=3FCA88C5", Crc32Ieee.Of(g) == 0x3FCA88C5u);
            check("crc32(empty)=00000000", Crc32Ieee.Of(new byte[0]) == 0);

            log.WriteLine("== LZO round-trip ==");
            Random rng = new Random(20261104);
            byte[][] cases = BuildLzoCases(rng);
            for (int i = 0; i < cases.Length; i++)
            {
                byte[] src = cases[i];
                byte[] comp = Lzo.Compress(src);
                byte[] back = new byte[src.Length];
                int got;
                string err = null;
                try { got = Lzo.Decompress(comp, back); }
                catch (LzoException e) { got = -1; err = e.Message; }
                bool ok = got == src.Length;
                if (ok) for (int k = 0; k < src.Length; k++) if (back[k] != src[k]) { ok = false; break; }
                check(string.Format("case {0} len={1} comp={2}{3}", i, src.Length, comp.Length, err == null ? "" : " (" + err + ")"), ok);
            }

            log.WriteLine("== bsdiff build/apply ==");
            DiffCase[] dcases = BuildDiffCases(rng);
            foreach (DiffCase dc in dcases)
            {
                bool ok = true;
                string err = null;
                try
                {
                    byte[] stream = Bsdiff.BuildStream(dc.OldB, dc.NewB);
                    byte[] patch = FakePatch(dc, stream);
                    byte[] applied = Bsdiff.ApplyPatch(dc.OldB, patch);
                    if (applied.Length != dc.NewB.Length) ok = false;
                    else for (int k = 0; k < applied.Length; k++) if (applied[k] != dc.NewB[k]) { ok = false; break; }
                }
                catch (Exception e) { ok = false; err = e.Message; }
                check(dc.Name + (err == null ? "" : " (" + err + ")"), ok);
            }

            if (foreignPatch != null && File.Exists(foreignPatch))
            {
                log.WriteLine("== foreign patch decode (real minilzo cross-validation) ==");
                try
                {
                    byte[] p = File.ReadAllBytes(foreignPatch);
                    PkgHeader h = Package.ParseDiffHeader(p);
                    byte[] logical = Bsdiff.ReadSegments(p, h, 0);
                    check(string.Format("foreign {0} B -> {1} B logical (all segments + cs verified)",
                        p.Length, logical.Length), true);
                }
                catch (Exception e) { check("foreign decode: " + e.Message, false); }
            }

            log.WriteLine(string.Format("== summary: {0} pass, {1} FAIL ==", pass, fail));
            return fail == 0 ? 0 : 1;
        }

        private sealed class DiffCase
        {
            public string Name; public byte[] OldB, NewB;
        }

        // wrap a logical stream into a minimal valid patch (header fields sane)
        private static byte[] FakePatch(DiffCase dc, byte[] logical)
        {
            // segment it raw so ApplyPatch's firmware walk is exercised directly
            var body = new List<byte>();
            for (int off = 0; off < logical.Length; off += 4096)
            {
                int n = Math.Min(4096, logical.Length - off);
                body.Add((byte)(n & 0xFF)); body.Add((byte)((n >> 8) & 0xFF));
                body.Add((byte)((n >> 16) & 0xFF)); body.Add(0);
                body.Add(0);                                        // raw
                body.Add(Bsdiff.CheckSum(logical, off, n));
                for (int i = 0; i < n; i++) body.Add(logical[off + i]);
            }
            byte[] bodyArr = body.ToArray();
            uint newCrc = Crc32Ieee.Of(dc.NewB);
            var p = new List<byte>();
            p.AddRange(Encoding.ASCII.GetBytes("DIFF"));
            Pu32(p, (uint)bodyArr.Length);
            Pu32(p, Crc32Ieee.Of(bodyArr));
            Pu32(p, 4096);
            Pu16(p, 0x0007); Pu16(p, 0xFFFF);
            Pu32(p, 0x0B00); Pu32(p, (uint)dc.NewB.Length); Pu32(p, newCrc);
            Pu32(p, 0x0A00); Pu32(p, (uint)dc.OldB.Length); Pu32(p, Crc32Ieee.Of(dc.OldB));
            Pu32(p, 0);
            p.AddRange(bodyArr);
            return p.ToArray();
        }

        private static void Pu32(List<byte> l, uint v) { l.Add((byte)v); l.Add((byte)(v >> 8)); l.Add((byte)(v >> 16)); l.Add((byte)(v >> 24)); }
        private static void Pu16(List<byte> l, uint v) { l.Add((byte)v); l.Add((byte)(v >> 8)); }

        private static byte[][] BuildLzoCases(Random rng)
        {
            var list = new List<byte[]>();

            // literal-run boundaries: first-byte form caps at 238
            list.Add(new byte[1]);
            list.Add(new byte[3]);
            list.Add(new byte[4]);
            list.Add(new byte[17]);
            list.Add(new byte[18]);
            list.Add(new byte[19]);
            list.Add(new byte[238]);
            list.Add(new byte[239]);
            list.Add(new byte[240]);
            list.Add(new byte[300]);
            list.Add(new byte[4096]);
            // zero-heavy (typical diff stream)
            byte[] z = new byte[4096];
            for (int i = 0; i < 4096; i += 37) z[i] = 1;
            list.Add(z);
            // patterned (match-friendly)
            byte[] pat = new byte[4096];
            for (int i = 0; i < 4096; i++) pat[i] = (byte)(i * 7 + 3);
            list.Add(pat);
            // repeats (M2 short offsets)
            byte[] rep = new byte[4096];
            for (int i = 0; i < 4096; i++) rep[i] = (byte)("ABCDEFGH"[i % 8]);
            list.Add(rep);
            // random (incompressible)
            for (int c = 0; c < 3; c++)
            {
                byte[] r = new byte[4096];
                rng.NextBytes(r);
                list.Add(r);
            }
            // M3 (>0x800) and M4 (>0x4000) offsets: unique block, gap, repeat
            byte[] far = new byte[20 * 1024];
            rng.NextBytes(far);
            for (int i = 0; i < 20 * 1024; i++) far[i] = 0x5A;      // compressible filler
            byte[] blk = new byte[100];
            rng.NextBytes(blk);
            Array.Copy(blk, 0, far, 0, 100);
            Array.Copy(blk, 0, far, 0x900, 100);                    // ~M3 range
            Array.Copy(blk, 0, far, 0x4001, 100);                   // M4 range
            list.Add(far);
            // long match (>33 -> length extension bytes)
            byte[] big = new byte[8192];
            rng.NextBytes(big);
            Array.Copy(big, 0, big, 4096, 4096);                    // 4KB match
            list.Add(big);
            // sweep sizes 0..40 mixed
            for (int n = 0; n <= 40; n++)
            {
                byte[] b = new byte[n];
                for (int i = 0; i < n; i++) b[i] = (byte)(i * 31 + ((i & 1) != 0 ? 0xAA : 0));
                list.Add(b);
            }
            return list.ToArray();
        }

        private static DiffCase[] BuildDiffCases(Random rng)
        {
            var list = new List<DiffCase>();
            byte[] a = new byte[20000];
            rng.NextBytes(a);
            for (int i = 0; i < 20000; i += 64) a[i] = 0;           // make it matchable

            byte[] same = (byte[])a.Clone();
            list.Add(new DiffCase { Name = "identical", OldB = a, NewB = same });

            byte[] few = (byte[])a.Clone();
            for (int i = 0; i < 40; i++) few[3000 + i * 7] ^= 0xFF;
            list.Add(new DiffCase { Name = "sparse edits", OldB = a, NewB = few });

            byte[] shifted = new byte[20000];
            Array.Copy(a, 37, shifted, 0, 20000 - 37);              // shifted window
            rng.NextBytes2(shifted, 19900, 100);
            list.Add(new DiffCase { Name = "shifted window", OldB = a, NewB = shifted });

            byte[] grown = new byte[24000];
            Array.Copy(a, 0, grown, 0, 20000);
            rng.NextBytes2(grown, 20000, 4000);                     // grown tail
            list.Add(new DiffCase { Name = "grown image", OldB = a, NewB = grown });

            byte[] shrunk = new byte[9000];
            Array.Copy(a, 0, shrunk, 0, 9000);
            list.Add(new DiffCase { Name = "shrunk image", OldB = a, NewB = shrunk });

            byte[] rnd1 = new byte[8192], rnd2 = new byte[8192];
            rng.NextBytes(rnd1); rng.NextBytes(rnd2);
            list.Add(new DiffCase { Name = "fully disjoint", OldB = rnd1, NewB = rnd2 });

            return list.ToArray();
        }
    }

    internal static class RandomExt
    {
        public static void NextBytes2(this Random r, byte[] b, int off, int len)
        {
            byte[] t = new byte[len];
            r.NextBytes(t);
            Array.Copy(t, 0, b, off, len);
        }
    }

    // =========================================================================
    // WinForms UI
    // =========================================================================
    internal sealed class ToolForm : Form
    {
        private readonly RadioButton rbDiff, rbFull;
        private readonly TextBox tbOld, tbNew, tbHw, tbSw, tbBaseHw, tbBaseSw, tbLog;
        private readonly Label lblOld, lblNew;
        private readonly Button btnOld, btnNew, btnGen, btnSelfTest;

        public ToolForm()
        {
            Text = "HKT 升级包生成工具（地磁 差分/全量）";
            FormBorderStyle = FormBorderStyle.FixedSingle;
            MaximizeBox = false;
            StartPosition = FormStartPosition.CenterScreen;
            ClientSize = new System.Drawing.Size(700, 660);

            Font = new System.Drawing.Font("Microsoft YaHei UI", 9F);

            int y = 12;
            rbDiff = new RadioButton { Text = "差分包（旧+新 → HKTU 差分发布包）", Checked = true, Bounds = new System.Drawing.Rectangle(16, y, 340, 22) };
            rbFull = new RadioButton { Text = "全量包（镜像 → HKTU 全量发布包）", Bounds = new System.Drawing.Rectangle(360, y, 320, 22) };
            rbDiff.CheckedChanged += delegate { ModeChanged(); };
            rbFull.CheckedChanged += delegate { ModeChanged(); };
            Controls.Add(rbDiff); Controls.Add(rbFull);
            y += 30;

            lblOld = new Label { Text = "底包固件 (旧 .bin):", Bounds = new System.Drawing.Rectangle(16, y + 3, 130, 20) };
            tbOld = new TextBox { Bounds = new System.Drawing.Rectangle(150, y, 440, 24), ReadOnly = true };
            btnOld = new Button { Text = "浏览…", Bounds = new System.Drawing.Rectangle(598, y - 1, 80, 26) };
            btnOld.Click += delegate { PickFile(tbOld, true); };
            Controls.Add(lblOld); Controls.Add(tbOld); Controls.Add(btnOld);
            y += 34;

            lblNew = new Label { Text = "目标固件 (新 .bin):", Bounds = new System.Drawing.Rectangle(16, y + 3, 130, 20) };
            tbNew = new TextBox { Bounds = new System.Drawing.Rectangle(150, y, 440, 24), ReadOnly = true };
            btnNew = new Button { Text = "浏览…", Bounds = new System.Drawing.Rectangle(598, y - 1, 80, 26) };
            btnNew.Click += delegate { PickFile(tbNew, false); };
            Controls.Add(lblNew); Controls.Add(tbNew); Controls.Add(btnNew);
            y += 34;

            var grp = new GroupBox { Text = "版本（hw/sw，与设备 BLE 上报一致；选文件后自动识别）", Bounds = new System.Drawing.Rectangle(16, y, 662, 58) };
            var l1 = new Label { Text = "目标 HW", Bounds = new System.Drawing.Rectangle(16, 24, 56, 20) };
            tbHw = new TextBox { Bounds = new System.Drawing.Rectangle(74, 21, 44, 24) };
            var l2 = new Label { Text = "SW", Bounds = new System.Drawing.Rectangle(124, 24, 26, 20) };
            tbSw = new TextBox { Bounds = new System.Drawing.Rectangle(152, 21, 44, 24) };
            var l3 = new Label { Text = "底包 HW", Bounds = new System.Drawing.Rectangle(250, 24, 58, 20) };
            tbBaseHw = new TextBox { Bounds = new System.Drawing.Rectangle(312, 21, 44, 24) };
            var l4 = new Label { Text = "SW", Bounds = new System.Drawing.Rectangle(362, 24, 26, 20) };
            tbBaseSw = new TextBox { Bounds = new System.Drawing.Rectangle(390, 21, 44, 24) };
            var l5 = new Label { Text = "分块 level=4096（固件 256–4096）", Bounds = new System.Drawing.Rectangle(452, 24, 200, 20) };
            grp.Controls.AddRange(new Control[] { l1, tbHw, l2, tbSw, l3, tbBaseHw, l4, tbBaseSw, l5 });
            Controls.Add(grp);
            y += 70;

            tbLog = new TextBox
            {
                Multiline = true,
                ReadOnly = true,
                ScrollBars = ScrollBars.Vertical,
                Bounds = new System.Drawing.Rectangle(16, y, 662, 410),
                Font = new System.Drawing.Font("Consolas", 9F),
                WordWrap = false,
            };
            Controls.Add(tbLog);
            y += 420;

            btnGen = new Button { Text = "生成发布包", Bounds = new System.Drawing.Rectangle(16, y, 120, 32) };
            btnGen.Click += delegate { Generate(); };
            btnSelfTest = new Button { Text = "自检", Bounds = new System.Drawing.Rectangle(146, y, 90, 32) };
            btnSelfTest.Click += delegate
            {
                tbLog.AppendText("运行自检…" + Environment.NewLine);
                var sw = new StringWriter();
                int rc = SelfTest.Run(sw, null);
                tbLog.AppendText(sw.ToString());
                tbLog.AppendText(rc == 0 ? "自检全部通过" + Environment.NewLine : "自检存在失败项！" + Environment.NewLine);
            };
            Controls.Add(btnGen); Controls.Add(btnSelfTest);

            ModeChanged();
        }

        private void ModeChanged()
        {
            bool diff = rbDiff.Checked;
            lblOld.Enabled = diff; tbOld.Enabled = diff; btnOld.Enabled = diff;
            tbBaseHw.Enabled = diff; tbBaseSw.Enabled = diff;
            lblNew.Text = diff ? "目标固件 (新 .bin):" : "固件镜像 (.bin):";
        }

        private void PickFile(TextBox tb, bool isOld)
        {
            using (OpenFileDialog d = new OpenFileDialog())
            {
                d.Filter = "固件镜像 (*.bin;*.img)|*.bin;*.img|所有文件 (*.*)|*.*";
                if (d.ShowDialog(this) != DialogResult.OK) return;
                tb.Text = d.FileName;
                if (isOld) ApplyGuess(tb, tbBaseHw, tbBaseSw);
                else
                {
                    ApplyGuess(tb, tbHw, tbSw);
                    try
                    {
                        int[] fi = Package.ParseFtkh(File.ReadAllBytes(d.FileName));
                        if (fi != null)
                        {
                            tbHw.Text = fi[0].ToString(); tbSw.Text = fi[1].ToString();
                            LogLine("内嵌 FTKH 记录 v" + fi[0] + "." + fi[1] + "（已覆盖文件名猜测）");
                        }
                        else LogLine("镜像偏移 0x200 无 FTKH 记录");
                    }
                    catch (Exception ex) { LogLine("读取失败: " + ex.Message); }
                }
            }
        }

        private void ApplyGuess(TextBox tb, TextBox hwBox, TextBox swBox)
        {
            if (tb.Text.Length == 0) return;
            int[] g = Package.GuessFromFilename(tb.Text);
            if (g != null) { hwBox.Text = g[0].ToString(); swBox.Text = g[1].ToString(); }
            else LogLine("文件名未识别出版本（形如 V11.28），请手动填写版本");
        }

        private void LogLine(string s) { tbLog.AppendText(s + Environment.NewLine); }

        private void Generate()
        {
            try
            {
                tbLog.Clear();
                byte[] bin;
                string outPath;
                PkgResult r;
                var log = new StringWriter();
                if (rbDiff.Checked)
                {
                    if (tbOld.Text.Length == 0 || tbNew.Text.Length == 0) throw new Exception("请先选择底包与目标固件");
                    byte[] oldB = File.ReadAllBytes(tbOld.Text);
                    bin = File.ReadAllBytes(tbNew.Text);
                    int hw = ParseBox(tbHw, "目标 HW"), sw = ParseBox(tbSw, "目标 SW");
                    int bhw = ParseBox(tbBaseHw, "底包 HW"), bsw = ParseBox(tbBaseSw, "底包 SW");
                    r = Package.MakeDiff(oldB, bin, hw, sw, bhw, bsw, log);
                    outPath = CombineOut(tbNew.Text, string.Format("_V{0}.{1}_diff.bin", hw, sw));
                }
                else
                {
                    if (tbNew.Text.Length == 0) throw new Exception("请先选择固件镜像");
                    bin = File.ReadAllBytes(tbNew.Text);
                    int hw = ParseBox(tbHw, "目标 HW"), sw = ParseBox(tbSw, "目标 SW");
                    r = Package.MakeFull(bin, hw, sw, log);
                    outPath = CombineOut(tbNew.Text, string.Format("_V{0}.{1}_full.bin", hw, sw));
                }
                File.WriteAllBytes(outPath, r.File);
                tbLog.AppendText(log.ToString());
                LogLine("------------------------------------------------------------");
                LogLine("已生成: " + outPath + "  (" + r.File.Length + " B)");
                LogLine("将该文件发给 APP「升级」页选择即可（APP 自动剥头发送载荷）");
            }
            catch (Exception ex)
            {
                tbLog.AppendText("生成失败: " + ex.Message + Environment.NewLine);
            }
        }

        private static string CombineOut(string src, string suffix)
        {
            string dir = Path.GetDirectoryName(src);
            string name = Path.GetFileNameWithoutExtension(src);
            return Path.Combine(dir, name + suffix);
        }

        private int ParseBox(TextBox tb, string what)
        {
            if (tb.Text.Trim().Length == 0) throw new Exception(what + " 未填写");
            int v = int.Parse(tb.Text.Trim());
            if (v < 0 || v > 255) throw new Exception(what + " 必须在 0..255");
            return v;
        }
    }
}
