package com.hkt.ble.bletools.core.ota

import com.hkt.ble.bletools.core.protocol.CRC16
import com.hkt.ble.bletools.core.protocol.u16be
import com.hkt.ble.bletools.core.protocol.u32be

/**
 * Bootloader OTA transfer framing.
 * Firmware authority: 各工程 Compents/BootLoader/uart.c（页大小 FLASH_ONE_PAGE_SIZE=2048B）
 * - frame: hkt(3) len(2 BE) cmd(1) packNum(2 BE) data(n) crc(2) "bootload"(8); `len - 15 == dataLen`
 * - data packets: 128 bytes fixed, packet numbers start at **0** (first device ACK is (0x02, 0));
 *   0x02 in the ACK is the command code, the following 2 bytes are the requested packet number.
 * - final chunk padded with FF to the 8-byte boundary (copy granularity only) and **sent with
 *   cmd 0xFF to force the flash write**: page size is 2048 B, so a non-2048-aligned image never
 *   fills the page buffer and without the flush the transfer never completes.
 * - completion ACK(cmd=0x03) does NOT jump — the host must follow with the cmd=3 finish frame
 *   (`finishFrame`) for `AppProgramRun()`; otherwise the device stays in the bootloader.
 * - after ~10 s of silence the device resets the transfer and replies ACK(cmd=0x01, 0) = restart.
 * Vectors: shared/fixtures/ota-transfer.json (OTA-*).
 */
data class OtaPacket(val index: Int, val chunk: ByteArray, val isFinal: Boolean)

object OTATransferPlanner {
    const val CHUNK_SIZE = 128
    private val bootloadSuffix = byteArrayOf(
        0x62, 0x6F, 0x6F, 0x74, 0x6C, 0x6F, 0x61, 0x64,
    )
    internal val bootloadSuffixBytes: ByteArray get() = bootloadSuffix.copyOf()

    /** Pads a final chunk with 0xFF up to the 8-byte boundary. */
    fun padded(chunk: ByteArray): ByteArray {
        var padded = chunk
        while (padded.size % 8 != 0) padded = padded + 0xFF.toByte()
        return padded
    }

    /**
     * Bootloader start frame (cmd 1, carries the firmware size in bytes):
     * `hkt len(2)=0005 cmd(1) size(4 BE) crc(2) bootload(8)` — no pack number.
     * Bootloader uart.c `fromBleDataHandle` case 1 reads size at data[6..9], answers
     * ACK(cmd+1=2, 0) and starts requesting packets. App firmware ignores this frame
     * (bootload suffix), so the sender must re-send it after the device reboots into
     * the bootloader (Android re-sends every 500 ms while ENTER_OTA is pending).
     */
    fun startFrame(sizeBytes: Int): ByteArray {
        val body = byteArrayOf(0x01) + u32be(sizeBytes.toLong())
        return bootloadFrame(body)
    }

    /**
     * Bootloader finish frame (cmd 3, no data/packNum): `hkt 0001 03 crc bootload` (16 B).
     * The ONLY jump-to-app trigger (uart.c case 3: InfoUartAck(3,0) → AppProgramRun);
     * the size-threshold completion path merely ACKs and never jumps — omitting this
     * frame leaves the device stuck in the bootloader. CRC covers the single cmd byte.
     */
    fun finishFrame(): ByteArray = bootloadFrame(byteArrayOf(0x03))

    /**
     * Builds one data packet frame. Regular packets use cmd 0x02; **the final packet must use
     * cmd 0xFF (`forceFlush: true`)**: the page buffer is 2048 B and only `cmd == 0xFF` (or a
     * full 2048 B page) triggers the flash write followed by the completion ACK(3,0) — a
     * partially-filled last packet sent as cmd 0x02 would stall the transfer forever.
     */
    fun dataFrame(packetIndex: Int, chunk: ByteArray, forceFlush: Boolean = false): ByteArray {
        val payload = padded(chunk)
        val body = byteArrayOf((if (forceFlush) 0xFF else 0x02).toByte()) + u16be(packetIndex) + payload
        return bootloadFrame(body)
    }

    /** Splits a validated firmware image into packets. */
    fun packets(image: ByteArray): List<OtaPacket> {
        val result = ArrayList<OtaPacket>()
        var offset = 0
        while (offset < image.size) {
            val end = minOf(offset + CHUNK_SIZE, image.size)
            val chunk = image.copyOfRange(offset, end)
            val isFinal = end == image.size
            result.add(OtaPacket(result.size, chunk, isFinal))
            offset = end
        }
        return result
    }

    /** Bootloader ACK frame shape (uart.c InfoUartAck): hkt cmd count(2) bootload — 14 B.
     *  Documented here because [OtaAck.parse] mirrors it. */
    private fun bootloadFrame(body: ByteArray): ByteArray {
        val frame = ByteArray(3 + 2 + body.size + 2 + bootloadSuffix.size)
        var o = 0
        frame[o++] = 0x68; frame[o++] = 0x6B; frame[o++] = 0x74
        val length = body.size
        frame[o++] = ((length shr 8) and 0xFF).toByte()
        frame[o++] = (length and 0xFF).toByte()
        body.copyInto(frame, o); o += body.size
        val crc = CRC16.ccitt(body)
        frame[o++] = ((crc shr 8) and 0xFF).toByte()
        frame[o++] = (crc and 0xFF).toByte()
        bootloadSuffix.copyInto(frame, o)
        return frame
    }
}

/** Parsed device bootloader ACK: hkt(3) cmd(1) count(2 BE) bootload(8) — 14 bytes. */
data class OtaAck(val kind: Kind, val requestedPacket: Int) {
    enum class Kind(val code: Int) {
        RESTART_TRANSFER(0x01),  // device reset the transfer: start from packet 0
        REQUEST_PACKET(0x02),    // device requests `requestedPacket`
        TRANSFER_COMPLETE(0x03),
    }

    companion object {
        fun parse(data: ByteArray): OtaAck? {
            if (data.size != 14) return null
            if (data[0] != 0x68.toByte() || data[1] != 0x6B.toByte() || data[2] != 0x74.toByte()) return null
            val suffix = OTATransferPlanner.bootloadSuffixBytes
            for (i in suffix.indices) {
                if (data[6 + i] != suffix[i]) return null
            }
            val kind = Kind.entries.firstOrNull { it.code == (data[3].toInt() and 0xFF) } ?: return null
            val packet = ((data[4].toInt() and 0xFF) shl 8) or (data[5].toInt() and 0xFF)
            return OtaAck(kind, packet)
        }
    }
}
