# Firmware Traceability Baseline

## 1. Purpose

iOS features, protocol contracts, test cases, UI states, and software design documents must be traceable to the firmware implementation that actually accepts and returns the corresponding data.

The Android app is a compatibility reference, not the protocol authority. When Android and firmware disagree, the firmware source and the selected firmware release define the implementable contract. Any intentional iOS-side restriction must be recorded separately.

## 2. Baseline

| Item | Value |
| --- | --- |
| Baseline branch | `nix/device-config-ux` |
| Baseline commit | `4e9f46211f6c01a007547f450b39ceaf01ad484b` |
| Baseline date | 2026-09-03 |
| Firmware source root | `HKT-Firmwares/` |

For later reviews, replace the commit with the firmware release tag or exact commit selected for each device. Do not use this document without a firmware baseline ID.

## 3. Device-to-Firmware Mapping

| App model | Firmware repository | Current product configuration | Core protocol files |
| --- | --- | --- | --- |
| UDS100 | `LoRaWAN_Ultrasonic_Distance_Sensor` | `USER/config.h` defines `PROCT_NAME "UDS100"`, hardware `0x02`, software `0x0A` | `USER/Drive/include/communicate.h`, `USER/Drive/communicate.c`, `Compents/BootLoader/uart.c` |
| DC200Family / EPS100 / MPS100 | `LoRaWAN_ParkingSensor` | The checked-out source currently selects `PROCT_NAME "MPS100"` and hardware `0x0B`; EPS100 requires the matching branch/build configuration with `PROCT_NAME "EPS100"` and hardware `0x0A` | `USER/config.h`, `USER/Drive/include/communicate.h`, `USER/Drive/communicate.c`, `Compents/BootLoader/uart.c` |
| SVC100 | `LoRaWAN_Solenoid_Valve_Controller` | `USER/config.h` defines `PROCT_NAME "SVC100"`, hardware `0x0D`, software `0x0D` | `USER/Drive/include/communicate.h`, `USER/Drive/communicate.c`, `Compents/BootLoader/uart.c` |

Firmware release artifacts in `USER/Firmware/` must not be treated as source evidence by themselves. Each OTA test target must be mapped to a source branch, tag, commit, or reproducible build configuration.

## 4. Protocol Evidence

### 4.1 Common transport

The application prefix `68 6B 74` and the Bootloader suffix `62 6F 6F 74 6C 6F 61 64` appear in all three firmware projects. The Bootloader parser checks CRC16 and rejects bad frames.

The normal application command handlers parse the binary `hkt` frames by command and payload length. The current application handlers were not observed to reject a frame solely because the Android-provided CRC bytes were wrong; nevertheless, iOS must continue to build and send CRC exactly like Android unless firmware explicitly declares CRC optional. iOS tests must distinguish these two statements:

1. iOS builds a valid CRC.
2. Firmware application code validates the CRC.

Only the first statement is currently proven by the Android behavior and App-frame implementation; the second must be verified per firmware release before being claimed in a test.

### 4.2 UDS100

| Capability | Firmware evidence | Contract finding |
| --- | --- | --- |
| Device identity | `USER/config.h`: `PROCT_NAME "UDS100"` | Advertised/model name is UDS100. |
| TLV types | `USER/Drive/include/communicate.h`: version `0x01`, battery voltage `0x8B`, temperature `0x09`, humidity `0x0A`, angle `0x0E`, slant `0x44`, HT alarm `0x28`, GPS period `0x45`, distance `0x46`, overflow state `0x47`, overflow config `0x48`, sync period `0x86`, power `0x8D`, ACK `0xFF` | The iOS parser must support these types as a device-specific view. |
| Status query | `USER/Drive/communicate.c`: `callback_BLESearch()` | Firmware returns version, power, battery voltage, temperature, humidity, angle, slant, HT alarm, GPS period, distance, overflow state, overflow config, and sync period. |
| Configuration | `fromBleDataHandle()`: command `0x02`, payload length `9` | Payload is report interval, GPS interval, low threshold, and high threshold. Firmware validates report `1-1440`, GPS `0` or `10-1440`, low threshold `30-4500`, and high threshold `0` or `30-4500`. |
| Power | `fromBleDataHandle()`: command `0xFE` | Payload byte selects power on/off; firmware persists the value and responds with ACK. |
| Calibration | `fromBleDataHandle()`: command `0xFD` | Firmware starts the accelerometer/angle offset mode and responds with ACK. |
| Time sync | `fromBleDataHandle()`: command `0x06` | Firmware reads a four-byte timestamp and adds the fixed UTC+8 offset. |
| OTA | `Compents/BootLoader/uart.c` | Bootloader handles size notification, packet request, 128-byte data packets, final padding, completion, and CRC validation. |

### 4.3 DC200Family / EPS100 / MPS100

| Capability | Firmware evidence | Contract finding |
| --- | --- | --- |
| Device identity | `USER/config.h`: current source selects `MPS100`; the disabled branch defines `EPS100` | EPS100 and MPS100 use the same firmware project but require separate product/build baselines. |
| TLV types | `USER/Drive/include/communicate.h`: version `0x01`, battery `0x03`, parking state `0x3A`, parking mode `0x3B`, sync time `0x80`, tamper `0x84`, sync period `0x86`, power `0x8D`, mag X/Y/Z `0x5D/0x5E/0x5F`, radar spectrum `0x60`, ACK `0xFF` | The iOS parser must expose these as the DC200Family status view. |
| Status query | `USER/Drive/communicate.c`: `callback_BLEQuery()` | Firmware returns version, power, battery, parking state, parking mode, tamper state, sync period, mag X/Y/Z, and ten radar spectrum values. |
| Configuration | `fromBleDataHandle()`: command `0x02`, payload length `4` | Payload is a two-byte report interval and one-byte parking mode. Firmware accepts report interval `0` or `1-1440`; the current Android UI exposes only `1-1440`. iOS must explicitly decide whether zero is hidden or exposed. |
| Calibration | `fromBleDataHandle()`: command `0xFD` | Firmware starts magnetometer calibration and responds with ACK; completion is asynchronous. |
| Power | `fromBleDataHandle()`: command `0xFE` | Firmware persists power state and responds with ACK. |
| Time sync | `fromBleDataHandle()`: command `0x06` | Firmware reads a four-byte timestamp and applies a fixed UTC+8 offset. |
| OTA | `Compents/BootLoader/uart.c` | Bootloader contract is common across the three firmware projects. |

### 4.4 SVC100

| Capability | Firmware evidence | Contract finding |
| --- | --- | --- |
| Device identity | `USER/config.h`: `PROCT_NAME "SVC100"` | Advertised/model name is SVC100. |
| TLV types | `USER/Drive/include/communicate.h`: version `0x01`, battery `0x03`, device state `0x3C`, voltage level `0x40`, port function `0x41`, stable time `0x42`, smart power `0x43`, sync time `0x80`, sync period `0x86`, timezone `0x8A`, power `0x8D`, ACK `0xFF` | The iOS parser must support these fields as the SVC100 status/configuration view. |
| Status query | `USER/Drive/communicate.c`: `callback_BLEQuery()` | Firmware returns version, power, battery, dual-valve state, voltage level, port function, stable time, smart power, sync period, and timezone. |
| Basic configuration | `fromBleDataHandle()`: command `0x02`, payload length `8` | Payload is voltage output, port function, stable time, smart power, timezone, and report interval. Firmware accepts report interval `0` or `1-1440`. |
| Realtime task | `fromBleDataHandle()`: command `0x03`, payload length `7` | Payload is valve, state, time, and pulse count. Firmware rejects execution while a local scheduled task is active. |
| Timed task | `fromBleDataHandle()`: command `0x04`, payload length `11` | Payload is task ID, valve, state, pulse count, start minute-of-day, end minute-of-day, and repeat mask. IDs are `1-16`; firmware validates valve, state, time, and repeat mask. |
| Delete timed task | `fromBleDataHandle()`: command `0x05`, payload length `2` | Payload is task ID; `0xFF` deletes all tasks. IDs are `1-16`, and firmware stops the active task before deleting it. |
| Time sync | `fromBleDataHandle()`: command `0x06` | Firmware applies the configured timezone, not a fixed UTC+8 offset. |
| Valve control | `fromBleDataHandle()`: command `0xF9` | Firmware supports a direct valve actuation path outside the Android v1 UI; iOS must record whether this remains out of scope. |
| OTA | `Compents/BootLoader/uart.c` | Bootloader contract is common across the three firmware projects. |

## 5. Traceability Rules

Every feature must carry the following links:

| Link | Meaning |
| --- | --- |
| `FR-*` | Functional requirement |
| `FW-REF-*` | Firmware repository, baseline, file, function, and behavior |
| `VEC-*` | Shared request/response or state-transition vector |
| `TC-*` | XCTest or integration test |
| `SD-*` | Software design section |
| `UX-*` | UI screen, component state, or interaction flow |

A feature may enter implementation only when all five links exist or the gap is explicitly accepted as an architecture-only extension.

Test vectors must include:

```json
{
  "id": "SVC-CONFIG-001",
  "device": "SVC100",
  "firmwareBaseline": "<tag-or-commit>",
  "androidReference": "android/app/src/main/java/com/hkt/ble/bletools/Communicate.kt#streamDevice(0x02)",
  "firmwareReference": "HKT-Firmwares/LoRaWAN_Solenoid_Valve_Controller/USER/Drive/communicate.c#fromBleDataHandle",
  "request": "<hex>",
  "expectedAck": "<hex>",
  "postCondition": "device reports the configured values",
  "iosTest": "SVC100ProtocolTests/testBasicConfigurationFrame"
}
```

## 6. Verification Gates

### 6.1 Contract gate

For each device:

1. Confirm model name from `USER/config.h`.
2. Enumerate supported TLV types from `USER/Drive/include/communicate.h`.
3. Extract status fields from the BLE query callback.
4. Extract accepted commands, payload lengths, field offsets, ranges, side effects, and ACK behavior from `fromBleDataHandle()`.
5. Extract persistence and time semantics.
6. Extract OTA behavior from `Compents/BootLoader/uart.c`.
7. Compare the result against Android and record every difference.

### 6.2 Test gate

1. Generate golden vectors for normal, boundary, and rejected values.
2. Add XCTest cases for frame construction and response parsing.
3. Add OTA state-machine cases for request, data packet, final padding, completion, reboot, reconnection, and version confirmation.
4. Add negative cases for malformed length, unknown command, invalid range, missing ACK, and CRC failure where applicable.
5. Record expected behavior when firmware logs an error but does not return a protocol-level negative ACK.

### 6.3 Design gate

The iOS software design must map every UI state and use case to:

1. A firmware reference.
2. A protocol command or response type.
3. A device capability flag.
4. A test case.
5. A ViewModel state.

The software design review is not complete if a feature exists only because Android has it.

## 7. Known Reconciliation Items

| Item | Firmware finding | Required action |
| --- | --- | --- |
| DC200Family report interval | Firmware accepts `0` or `1-1440`; current Android UI exposes `1-1440` | Decide whether iOS hides zero or exposes it; document the restriction and add both firmware and UI tests. |
| SVC100 timezone values | Firmware encodes `25` as `+03:30` and `26` as `+05:30`; values `13-24` are negative offsets | Add boundary vectors for all positive, negative, and half-hour cases. |
| SVC100 realtime task | Firmware refuses to start while a local scheduled task is active | Add UI and state-machine handling for busy/rejected execution. |
| SVC100 direct valve control | Firmware supports command `0xF9`; current Android v1 UI does not expose it | Treat as an explicit architecture capability until product confirms scope. |
| Application-frame CRC | Android sends CRC; current application handlers were not observed to validate it; Bootloader does validate CRC | Continue sending valid CRC; do not document application CRC rejection as firmware behavior until proven. |
| EPS100 source baseline | Current ParkingSensor checkout selects MPS100 | Select or confirm the EPS100 branch/build baseline before EPS100 OTA certification. |
