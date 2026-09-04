# Bootloader OTA Frame

The bootloader frame uses the ASCII prefix `686B74` and suffix `626F6F746C6F6164` (`bootload`).

```text
hkt(3) + len(2) + cmd(1) + packnum(2) + data(n) + crc(2) + bootloader(8)
```

Transfer lifecycle:

1. Send command `0x01` with the firmware size in bytes.
2. The bootloader ACKs with command code `0x02` and the requested packet number `0x0000`. Note: in the ACK, `0x02` is the command code; the packet number is the following 2 bytes. Subsequent ACKs request the next sequential packet number (0x0001, 0x0002, ...).
3. Send the requested packet: 128 bytes of firmware data under command `0x02` (stop-and-wait: never send a packet the bootloader has not requested).
4. Pad the final chunk to an 8-byte boundary with `FF`.
5. Send command `0x03` to finish.

App acceptance is successful only when the device reboots, re-advertises, the app reconnects, and reads the expected new firmware version. A progress value of 100% alone is not acceptance.
