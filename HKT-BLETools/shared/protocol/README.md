# HKT BLE Protocol

This directory contains the platform-neutral protocol contract and test vectors shared by the Android and iOS implementations.

- `app-frame.md`: normal command and response framing.
- `bootloader-ota.md`: firmware-update framing and transfer lifecycle.
- `crc16.md`: CRC algorithm and canonical examples.

The first iOS milestone must implement these contracts against the same fixtures used by Android protocol tests.

All contracts must be reconciled with the firmware source baseline in [`../devices/firmware-traceability.md`](../devices/firmware-traceability.md). The Android implementation is a compatibility reference, not the authority for firmware behavior.
