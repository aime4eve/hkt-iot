# App Frame

The application frame uses the ASCII prefix `686B74` (`hkt`).

## App → device (command)

```text
hkt(3) + packnum(1) + len(2 BE) + cmd(1) + data(n) + crc(2)
```

- `len` is the byte length of `cmd + data` (big-endian).
- `crc` is CRC16-CCITT (KERMIT) over `cmd + data`.
- Status query, power, calibration and OTA-notify carry a 4-byte filler payload
  (`0xFF` fill; power byte sits at payload[3] — firmware reads frame offset 10).

## Device → app (response)

```text
hkt(3) + 0x00 + seq(1) + record stream
```

- Records are `type(1) + fixed-size value` per type — there is **no length byte,
  no command byte and no CRC** in device responses (firmware `setDataPackage`).
- A response carries multiple records; parsers must dispatch by type with
  per-family fixed sizes. An unknown type cannot be skipped safely (no length
  field) — treat the stream as data-abnormal instead of skipping.
- All multi-byte values are big-endian; signed values use the sign-bit rules
  (16/24/32-bit). The ACK record is `0xFF 0xFF`.
