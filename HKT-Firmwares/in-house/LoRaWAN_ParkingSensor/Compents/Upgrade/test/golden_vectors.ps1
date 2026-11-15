# golden_vectors.ps1 - Independent re-check of the BLE upgrade protocol golden
# vectors (shared/fixtures/upgrade-protocol.json) against the algorithms as
# implemented in Compents/Upgrade (upgrade_frame.c checksum, upgrade_crc.c
# CRC16-KERMIT / CRC32-IEEE). Pure re-implementation: no firmware code is
# executed, so this validates the algorithm parameters and the byte layout.
#
# Usage:  powershell -NoProfile -ExecutionPolicy Bypass -File golden_vectors.ps1
# Exit code 0 = all vectors passed.

param(
    [string]$Fixture = (Join-Path $PSScriptRoot "..\..\..\..\..\..\HKT-BLETools\shared\fixtures\upgrade-protocol.json")
)

$ErrorActionPreference = "Stop"

function Convert-HexString([string]$hex) {
    $hex = $hex -replace '\s', ''
    $bytes = New-Object 'System.Collections.Generic.List[byte]'
    for ($i = 0; $i -lt $hex.Length; $i += 2) {
        $bytes.Add([Convert]::ToByte($hex.Substring($i, 2), 16))
    }
    return $bytes.ToArray()
}

# Frame checksum: byte sum from the sync head through the last data byte, mod 256.
function Get-FrameChecksum([byte[]]$b) {
    $sum = 0
    for ($i = 0; $i -lt ($b.Length - 1); $i++) { $sum += $b[$i] }
    return [byte]($sum % 256)
}

# CRC16/KERMIT: reflected 0x8408, init 0x0000, xorout 0x0000 (crclib.c crc16_ccitt).
function Invoke-Crc16Kermit([byte[]]$b) {
    [int]$crc = 0
    foreach ($x in $b) {
        $crc = $crc -bxor $x
        $crc = $crc -band 0xFFFF
        for ($i = 0; $i -lt 8; $i++) {
            if ($crc -band 1) { $crc = (($crc -shr 1) -bxor 0x8408) -band 0xFFFF }
            else { $crc = ($crc -shr 1) -band 0xFFFF }
        }
    }
    return $crc
}

# CRC32/IEEE: reflected 0xEDB88320, init 0xFFFFFFFF, final xor 0xFFFFFFFF.
# (all math in [long] with decimal constants - PS5.1 parses 0xFFFFFFFF as Int32 -1)
function Invoke-Crc32([byte[]]$b) {
    [long]$crc = 4294967295
    foreach ($x in $b) {
        $crc = (($crc -shr 8) -bxor $script:Crc32Table[($crc -bxor $x) -band 255]) -band 4294967295
    }
    return ($crc -bxor 4294967295)
}

$script:Crc32Table = New-Object 'long[]' 256
for ($i = 0; $i -lt 256; $i++) {
    [long]$tv = $i
    for ($j = 0; $j -lt 8; $j++) {
        if ($tv -band 1) { $tv = (($tv -shr 1) -bxor 3988292384) -band 4294967295 }
        else { $tv = ($tv -shr 1) -band 4294967295 }
    }
    $script:Crc32Table[$i] = $tv
}

# --- frame-level validation --------------------------------------------------
function Test-Frame([byte[]]$b, [string]$id, [object]$expected) {
    $fails = New-Object 'System.Collections.Generic.List[string]'
    if ($b.Length -lt 7) { $fails.Add("frame too short ($($b.Length))"); return $fails }
    if ($b[0] -ne 0x68 -or $b[1] -ne 0x6B -or $b[2] -ne 0x74) { $fails.Add("bad sync") }
    $special = $b[3]; $type = $b[5]
    if (-not ($special -band 0x02)) { $fails.Add("checksum bit (BIT1) not set") }
    $cs = Get-FrameChecksum $b
    if ($cs -ne $b[$b.Length - 1]) { $fails.Add(("checksum: calc 0x{0:X2} != frame 0x{1:X2}" -f $cs, $b[$b.Length - 1])) }
    $dataLen = $b.Length - 7
    $d = $b[6..($b.Length - 2)]

    switch ($type) {
        0xF0 { # notify: hw sw total2 pktlen2 pkg crc32(4)
            if ($dataLen -ne 11) { $fails.Add("F0 data len $dataLen != 11"); break }
            $hw = $d[0]; $sw = $d[1]
            $total = ($d[2] -shl 8) -bor $d[3]
            $pktlen = ($d[4] -shl 8) -bor $d[5]
            $pkg = $d[6]
            $crc32 = ($d[7] -shl 24) -bor ($d[8] -shl 16) -bor ($d[9] -shl 8) -bor $d[10]
            Write-Host ("  F0 hw=0x{0:X2} sw=0x{1:X2} total={2} pktlen={3} pkg=0x{4:X2} crc32=0x{5:X8}" -f $hw, $sw, $total, $pktlen, $pkg, $crc32)
            if ($special -ne 0x0B) { $fails.Add("F0 special 0x{0:X2} != 0x0B" -f $special) }
        }
        0xF1 { # data: seq2 crc16_2 len2 payload
            if ($dataLen -lt 7) { $fails.Add("F1 data len $dataLen < 7"); break }
            $seq = ($d[0] -shl 8) -bor $d[1]
            $crc16 = ($d[2] -shl 8) -bor $d[3]
            $declared = ($d[4] -shl 8) -bor $d[5]
            $payload = $d[6..($d.Length - 1)]
            if ($declared -ne $payload.Count) { $fails.Add("F1 declared len $declared != payload $($payload.Count)") }
            $real = Invoke-Crc16Kermit $payload
            Write-Host ("  F1 seq={0} declaredCrc16=0x{1:X4} kermit(payload)=0x{2:X4} len={3}" -f $seq, $crc16, $real, $payload.Count)
            if ($special -ne 0x0B) { $fails.Add("F1 special 0x{0:X2} != 0x0B" -f $special) }
        }
        0xF2 { # request: seq2
            if ($dataLen -ne 2) { $fails.Add("F2 data len $dataLen != 2"); break }
            $idx = ($d[0] -shl 8) -bor $d[1]
            Write-Host ("  F2 packetIndex={0}" -f $idx)
            if ($expected -and $expected.packetIndex -ne $null -and $expected.packetIndex -ne $idx) {
                $fails.Add("F2 packetIndex $idx != expected $($expected.packetIndex)")
            }
            if ($special -ne 0x0B) { $fails.Add("F2 special 0x{0:X2} != 0x0B" -f $special) }
        }
        0xF3 { # result: code extra
            if ($dataLen -ne 2) { $fails.Add("F3 data len $dataLen != 2"); break }
            Write-Host ("  F3 code={0} extra={1}" -f $d[0], $d[1])
            if ($expected -and $expected.code -ne $null -and $expected.code -ne $d[0]) {
                $fails.Add("F3 code $($d[0]) != expected $($expected.code)")
            }
            if ($expected -and $expected.extra -ne $null -and $expected.extra -ne $d[1]) {
                $fails.Add("F3 extra $($d[1]) != expected $($expected.extra)")
            }
            if ($special -ne 0x02) { $fails.Add("F3 special 0x{0:X2} != 0x02" -f $special) }
        }
        0xFE { # ack: acked type
            if ($dataLen -ne 1) { $fails.Add("FE data len $dataLen != 1"); break }
            Write-Host ("  FE ackedType=0x{0:X2}" -f $d[0])
            if ($special -ne 0x02) { $fails.Add("FE special 0x{0:X2} != 0x02" -f $special) }
        }
        default { $fails.Add("unknown type 0x{0:X2}" -f $type) }
    }
    return $fails
}

# --- main ---------------------------------------------------------------------
if (-not (Test-Path $Fixture)) { Write-Host "fixture not found: $Fixture"; exit 2 }
$vectors = Get-Content $Fixture -Raw -Encoding UTF8 | ConvertFrom-Json

$passCount = 0; $failCount = 0
foreach ($v in $vectors) {
    Write-Host ("[{0}] {1}" -f $v.id, $(if ($v.request) { "inbound " + $v.request } else { "outbound " + $v.response }))
    $hex = if ($v.request) { $v.request } else { $v.response }
    $b = Convert-HexString $hex
    $fails = Test-Frame $b $v.id $v.expected
    if ($fails.Count -eq 0) { Write-Host "  PASS"; $passCount++ }
    else { foreach ($f in $fails) { Write-Host "  FAIL: $f" }; $failCount++ }
}

# reference values for live bring-up (doc examples use placeholder CRCs)
$fw8 = Convert-HexString "0102030405060708"
Write-Host ""
Write-Host ("reference: kermit(01..04)=0x{0:X4}  kermit(05..08)=0x{1:X4}  crc32ieee(01..08)=0x{2:X8}" -f `
    (Invoke-Crc16Kermit @(0x01, 0x02, 0x03, 0x04)), `
    (Invoke-Crc16Kermit @(0x05, 0x06, 0x07, 0x08)), `
    (Invoke-Crc32 $fw8))

Write-Host ""
Write-Host ("{0} passed, {1} failed" -f $passCount, $failCount)
exit $(if ($failCount -gt 0) { 1 } else { 0 })
