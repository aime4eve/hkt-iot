# LoRaWAN Payload Decoder Toolkit

This repository provides tools to decode and debug LoRaWAN payloads reported by IoT devices such as water meters, valves, collectors, and pressure devices.

## Project Structure

- `uni-decode-cn.js`: Unified protocol decoder (Chinese output) for ChirpStack v4.
- `uni-decode-en.js`: Unified protocol decoder (English output) for ChirpStack v4. This is the default decoder used by `test-node.js`.
- `LoRaWAN-tools.html`: Single-file web parser with visual debugging.
- `Sample_chirpstack.js`: Example decoder for a level sensor (5-byte frame, sensor type `0x05`).
- `test-node.js`: Local Node.js test runner. Reads `encode-test.txt` and writes results to `test-result.txt`.
- `encode-test.txt`: Test payloads, one hex payload per line.
- `test-result.txt`: Generated test output file.
- `backup/`: Historical decoder versions and archives.

## Quick Start

### 1) Run local tests

From the project root:

```bash
node test-node.js
```

This command will:

- Read all valid lines from `encode-test.txt`
- Decode each payload using `uni-decode-en.js` (`decodeUplink`)
- Print summary output to terminal
- Save both summary and full `data` JSON for each case to `test-result.txt`

### 2) Check the output file

- Output path: `test-result.txt`
- Content includes:
  - Case index and raw payload
  - Decode summary (`afn`, `afnText`, `alarmCount`, `dataBlocks`)
  - Warning fields (if any)
  - Full `Data JSON` (`result.data`)

## Input and Output

### Input format (`encode-test.txt`)

- Plain text file
- One hex payload per line
- Empty lines are ignored

Example:

```text
663a01f35463647428508904c1150104a8bc6902188100000000a2220000000084000000004917079801000800008ebc690409a12300000000430000e216
663601F35463647429311104C103013080B8690123414A0EC42603162212320709383639353331313030313638353934304943232F0433213616
```

### Decoder return format

The decoder follows ChirpStack v4 codec style:

```javascript
function decodeUplink(input) {
  // input.bytes: Uint8Array
  // return: { data: { ... } }
}
```

Typical return object:

```json
{
  "data": {
    "byteLength": 62,
    "payloadHex": "....",
    "afn": 1,
    "afnText": "Real-time Data Report"
  }
}
```

`data` is a flat object and usually contains:

- Frame fields: `startFlag`, `length`, `version`, `address`, `seq`, `deviceType`, `control`, `ci`, `cs`, `endFlag`
- AFN/control fields: `afn`, `afnText`, `dirText`, `prmText`
- Statistics: `alarmCount`, `dataBlocks`
- Parsed item fields with dynamic keys
- Error mode: `{ data: { error: "..." } }`

## ChirpStack v4 Integration

You can paste the code from `uni-decode-cn.js` or `uni-decode-en.js` into your ChirpStack Device Profile codec configuration:

- Device Profiles
- Codec
- Custom JavaScript codec functions

## Protocol Notes

- Endianness: Little Endian
- Frame format:

```text
Start(0x66) | Length L | Version V | Address A(7 bytes) | SEQ | Device Type | C | CI | Data Area | CS | End(0x16)
```

- AFN (low 4 bits of C):
  - `0x01`: Real-time data report
  - `0x03`: Alarm data report (Fn=6)
  - `0x04`: Parameter setting
  - `0x05`: Parameter reading
  - `0x06`: Control command

## Environment

- Node.js 14+ (LTS recommended)
- macOS / Linux / Windows
