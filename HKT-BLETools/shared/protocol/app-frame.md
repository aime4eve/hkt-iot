# App Frame

The application frame uses the ASCII prefix `686B74` (`hkt`).

```text
hkt(3) + packnum(1) + len(2) + cmd(1) + data(n) + crc(2)
```

- `len` is the byte length of `cmd + data`.
- `crc` is CRC16-CCITT over `cmd + data`.
- Responses can contain multiple TLV records after the frame header.
- Command `0xFF` is the periodic status query and also acknowledges completed control operations.

The iOS protocol parser must preserve the Android parser's multi-record behavior rather than treating every response as a single-value packet.
