<#
pack_hktu.ps1 - wrap a raw PC-diff-tool patch into the HKTU release package
that the Android upgrade screen accepts (UpgradePackage.kt "release package
with header").

Usage:
  powershell -ExecutionPolicy Bypass -File pack_hktu.ps1 <raw_patch.bin> [output_file]

  <raw_patch.bin>   the file the PC diff tool produced (48B DIFF header + LZO body)
  [output_file]     optional; default = <input>_hktu.bin next to the input

The Android app strips this header and sends only the payload, so the device
still receives the raw patch bytes - firmware side is unaffected.

HKTU header layout, 24 bytes fixed, multi-byte fields LITTLE-endian
(must stay in sync with UpgradePackage.kt):
  off  len  field
  0x00  4   magic "HKTU"
  0x04  1   header version = 1
  0x05  1   package type (1 = diff)
  0x06  2   target hw version            <- DIFF header new_fw_version low 16: (hw<<8|sw)
  0x08  2   target sw version            <- DIFF header new_fw_version low 16: sw byte
  0x0A  2   base sw version (0xFFFF=n/a) <- DIFF header old_fw_version low 16: sw byte
  0x0C  4   base image CRC32             <- DIFF header old_fw_crc
  0x10  4   payload CRC32                <- CRC32-IEEE over the whole raw patch file
  0x14  4   payload length in bytes

DIFF patch header offsets (packed, little-endian; must match
upgrade_common.h upg_patch_header_t):
  0 magic[4]  4 patch_size  8 patch_crc  12 level  16 pid  18 cid
  20 new_fw_version  24 new_fw_size  28 new_fw_crc
  32 old_fw_version  36 old_fw_size  40 old_fw_crc  44 reserved[4]
#>
param(
    [Parameter(Mandatory = $true)][string]$PatchPath,
    [string]$OutPath
)

$ErrorActionPreference = "Stop"

# ---- firmware-side bounds (upgrade_common.h) ----
$PID_EXPECT      = 0x0007          # shared PC diff tool product id (hydrant toolchain)
$CID_EXPECT      = 0xFFFF
$LEVEL_MIN       = 256
$LEVEL_MAX       = 4096
$APP_MAX_SIZE    = 0x2A000         # 168KB image cap (user params above 0x08030000)
$PATCH_DATA_MAX  = 0x03F000        # ext-flash Patch area minus the 4KB session sector
$HDR_SIZE        = 24              # HKTU header
$DIFF_HDR_SIZE   = 48              # DIFF patch header

function Get-U16([byte[]]$b, [int]$off) {
    return [int]$b[$off] -bor ([int]$b[$off + 1] -shl 8)
}

function Get-U32([byte[]]$b, [int]$off) {
    # compose as uint32 throughout: int -shl 24 goes negative past 0x7FFFFFFF,
    # and PS refuses casting a negative int to [uint32]
    return [uint32]([uint32]$b[$off] -bor ([uint32]$b[$off + 1] -shl 8) -bor
        ([uint32]$b[$off + 2] -shl 16) -bor ([uint32]$b[$off + 3] -shl 24))
}

# CRC32-IEEE (reflected 0xEDB88320) - same as firmware upg_crc32 / Kotlin CRC32.ieee.
# NB: hex literal 0xFFFFFFFF parses as int32 -1 in PS and [uint32]-1 throws;
# 4294967295 parses as int64 and casts cleanly.
$script:crcTable = $null
function Get-Crc32([byte[]]$data) {
    if ($null -eq $script:crcTable) {
        $script:crcTable = New-Object 'uint32[]' 256
        for ($i = 0; $i -lt 256; $i++) {
            $c = [uint32]$i
            for ($k = 0; $k -lt 8; $k++) {
                if ($c -band 1) { $c = [uint32](($c -shr 1) -bxor 0xEDB88320) }
                else { $c = [uint32]($c -shr 1) }
            }
            $script:crcTable[$i] = $c
        }
    }
    $c = [uint32]4294967295
    foreach ($b in $data) {
        $idx = [int](($c -bxor $b) -band 0xFF)
        $c = [uint32](($c -shr 8) -bxor $script:crcTable[$idx])
    }
    return [uint32]($c -bxor 4294967295)
}

# ----------------------------------------------------------------------------
# 1. load + validate the raw patch's 48B DIFF header (device-side gate mirror)
# ----------------------------------------------------------------------------
if (-not (Test-Path -LiteralPath $PatchPath)) { Write-Error "patch not found: $PatchPath" }
$patch = [System.IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $PatchPath).Path)

if ($patch.Length -lt $DIFF_HDR_SIZE + 1) { Write-Error "patch too small: $($patch.Length) B (must exceed the 48B DIFF header)" }
$magic = [System.Text.Encoding]::ASCII.GetString($patch, 0, 4)
if ($magic -ne "DIFF") { Write-Error ("bad DIFF magic: '{0}' - not a PC diff tool output?" -f $magic) }

$patchSize  = Get-U32 $patch 4
$patchCrc   = Get-U32 $patch 8
$level      = Get-U32 $patch 12
$pidVal     = Get-U16 $patch 16
$cid        = Get-U16 $patch 18
$newVer     = Get-U32 $patch 20
$newSize    = Get-U32 $patch 24
$newCrc     = Get-U32 $patch 28
$oldVer     = Get-U32 $patch 32
$oldSize    = Get-U32 $patch 36
$oldCrc     = Get-U32 $patch 40

if ($patchSize -ne $patch.Length - $DIFF_HDR_SIZE) {
    Write-Error ("patch_size field {0} != file-48 ({1}) - truncated or padded file?" -f $patchSize, ($patch.Length - $DIFF_HDR_SIZE))
}
if ($pidVal -ne $PID_EXPECT)  { Write-Error ("pid 0x{0:X4} != 0x{1:X4} (device rejects it; regenerate with the shared tool)" -f $pidVal, $PID_EXPECT) }
if ($cid -ne $CID_EXPECT)  { Write-Error ("cid 0x{0:X4} != 0x{1:X4}" -f $cid, $CID_EXPECT) }
if ($level -lt $LEVEL_MIN -or $level -gt $LEVEL_MAX) { Write-Error ("level {0} outside {1}..{2}" -f $level, $LEVEL_MIN, $LEVEL_MAX) }
if ($newSize -gt $APP_MAX_SIZE) { Write-Error ("new_fw_size {0} > APP cap {1}" -f $newSize, $APP_MAX_SIZE) }
if ($patch.Length -gt $PATCH_DATA_MAX) { Write-Error ("patch {0} B > ext Patch area {1} B" -f $patch.Length, $PATCH_DATA_MAX) }

# whole-patch CRC must equal the F0 file CRC the app will declare (payload CRC)
$bodyCrc = Get-Crc32 $patch
if ($bodyCrc -ne $patchCrc) {
    Write-Warning ("note: whole-file CRC32 {0:X8} != header patch_crc {1:X8} (firmware only checks the declared one)" -f $bodyCrc, $patchCrc)
}

# ----------------------------------------------------------------------------
# 2. build the 24B HKTU header + payload
# ----------------------------------------------------------------------------
$hw     = ($newVer -band 0xFFFF) -shr 8
$sw     =  $newVer -band 0xFF
$baseHw = ($oldVer -band 0xFFFF) -shr 8
$baseSw =  $oldVer -band 0xFF

if (-not $OutPath) {
    $dir  = Split-Path -Parent (Resolve-Path -LiteralPath $PatchPath).Path
    $name = [System.IO.Path]::GetFileNameWithoutExtension($PatchPath)
    $OutPath = Join-Path $dir ($name + "_hktu.bin")
}

$h = New-Object 'byte[]' $HDR_SIZE
$h[0] = 0x48; $h[1] = 0x4B; $h[2] = 0x54; $h[3] = 0x55      # "HKTU"
$h[4] = 1                                                    # header version
$h[5] = 1                                                    # package type: diff
$h[6] = [byte]($hw -band 0xFF);      $h[7] = [byte]($hw -shr 8)
$h[8] = [byte]($sw -band 0xFF);      $h[9] = [byte]($sw -shr 8)
$h[10] = [byte]($baseSw -band 0xFF); $h[11] = [byte]($baseSw -shr 8)
foreach ($i in 0..3) { $h[12 + $i] = [byte](($oldCrc  -shr (8 * $i)) -band 0xFF) }
foreach ($i in 0..3) { $h[16 + $i] = [byte](($bodyCrc -shr (8 * $i)) -band 0xFF) }
$len32 = [uint32]$patch.Length
foreach ($i in 0..3) { $h[20 + $i] = [byte](($len32 -shr (8 * $i)) -band 0xFF) }

$out = New-Object 'byte[]' ($HDR_SIZE + $patch.Length)
[Array]::Copy($h, 0, $out, 0, $HDR_SIZE)
[Array]::Copy($patch, 0, $out, $HDR_SIZE, $patch.Length)
[System.IO.File]::WriteAllBytes($OutPath, $out)

# ----------------------------------------------------------------------------
# 3. self-verify: re-parse exactly like the Android UpgradePackage parser does
# ----------------------------------------------------------------------------
$back = [System.IO.File]::ReadAllBytes($OutPath)
$okMagic = ($back[0] -eq 0x48 -and $back[1] -eq 0x4B -and $back[2] -eq 0x54 -and $back[3] -eq 0x55)
$okVer   = ($back[4] -eq 1)
$okType  = ($back[5] -eq 1)
$okLen   = ((Get-U32 $back 20) -eq $back.Length - $HDR_SIZE)
$payload = New-Object 'byte[]' ($back.Length - $HDR_SIZE)
[Array]::Copy($back, $HDR_SIZE, $payload, 0, $payload.Length)
$okCrc   = ((Get-U32 $back 16) -eq (Get-Crc32 $payload))
if (-not ($okMagic -and $okVer -and $okType -and $okLen -and $okCrc)) {
    Write-Error "self-verify FAILED (magic=$okMagic ver=$okVer type=$okType len=$okLen crc=$okCrc)"
}

$oldCrcHex = "{0:X8}" -f $oldCrc
Write-Host ""
Write-Host ("patch   : {0} ({1} B)" -f $PatchPath, $patch.Length)
Write-Host ("header  : pid=0x{0:X4} cid=0x{1:X4} level={2}" -f $pidVal, $cid, $level)
Write-Host ("versions: target v{0}.{1}  base v{2}.{3}  (old_fw_crc={4:X8})" -f $hw, $sw, $baseHw, $baseSw, $oldCrc)
Write-Host ("image   : new_fw_size={0}  new_fw_crc={1:X8}" -f $newSize, $newCrc)
Write-Host ("release : {0} ({1} B)  payload CRC32={2:X8}" -f $OutPath, $out.Length, $bodyCrc)
Write-Host "self-verify: PASS (magic/version/type/len/crc re-parsed like the app)"
Write-Host ""
Write-Host "NOTE: the device must be running the base version (v$baseHw.$baseSw), and the"
Write-Host "      old .bin given to the diff tool must be byte-identical to the running"
Write-Host "      image (old_fw_crc=$oldCrcHex), otherwise device-side synthesis rejects it."
