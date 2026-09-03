# Bootloader OTA Frame

The bootloader frame uses the ASCII prefix `686B74` and suffix `626F6F746C6F6164` (`bootload`).

```text
hkt(3) + len(2) + cmd(1) + packnum(2) + data(n) + crc(2) + bootloader(8)
```

Transfer lifecycle:

1. Send command `0x01` with the firmware size in bytes.
2. The bootloader requests packet number `0x0002`.
3. Send 128-byte data chunks under command `0x02`.
4. Pad the final chunk to an 8-byte boundary with `FF`.
5. Send command `0x03` to finish.

App acceptance is successful only when the device reboots, re-advertises, the app reconnects, and reads the expected new firmware version. A progress value of 100% alone is not acceptance.
