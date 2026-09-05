# Bootloader OTA Frame

The bootloader frame uses the ASCII prefix `686B74` and suffix `626F6F746C6F6164` (`bootload`).

```text
hkt(3) + len(2) + cmd(1) + packnum(2) + data(n) + crc(2) + bootloader(8)
```

Transfer lifecycle:

1. Send app-frame command `0x01` — the device writes the update flag and reboots into the bootloader.
2. The bootloader ACKs `hkt 02 00 00 bootload`: command code `0x02` requesting packet number `0x0000` (packet numbers start at 0; `0x02` is the command code, not the packet number).
3. Send the requested packet: 128 bytes under command `0x02` (stop-and-wait — never send a packet the bootloader has not requested; expect longer ACK latency every 16 packets, a 2 KB flash page erase+write).
4. Pad the final chunk minimally with `FF` to the 8-byte boundary.
5. Completion is device-driven: once `firmware_write_size >= firmware_size` the device flushes and replies ACK command `0x03` (a packet sent with command `0xFF` forces the flush). After ~10 s of silence the device resets the transfer and replies ACK `0x01, 0` = start over.

App acceptance is successful only when the device reboots, re-advertises, the app reconnects, and reads the expected new firmware version. A progress value of 100% alone is not acceptance.
