# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BLETools is an Android (Kotlin) app that scans, connects to, configures, and firmware-updates a family of BLE IoT devices over a custom binary protocol. The supported device families are:

- **UDS100** — ultrasonic trash-bin overflow sensor
- **DC200** — geomagnetic / radar parking sensor (also matches names `DC201`, `EPS100`, `PS100`, `E_`)
- **SVC100** — dual solenoid-valve controller
- **MPS100** — (matches `M_` prefix)

Device type is resolved from the advertised name by `MainActivity.parseDeviceType()` into a `DeviceNameEnum`. The same Activity/protocol code serves all types — the device type drives which command/data fields are sent and which UI sections render in `DeviceActivity`.

## Build & Run

**Stack:** Kotlin 1.8.0, AGP 7.4.2, Gradle 8.5 (wrapper), compileSdk/targetSdk 34, minSdk 26, JDK 17 (jvmTarget 17).

**JDK 17 is a hard requirement.** `gradle.properties` pins `org.gradle.java.home=D:\Java\jdk-17` and `build.ps1` force-sets `JAVA_HOME` to JDK 17 before invoking gradlew — this exists specifically to avoid an invalid `jdk-21` on the PATH. On this Windows machine, prefer `./build.ps1` (or `./gradlew.bat`) over the Unix `./gradlew`.

```bash
# Build variants: {dev,prod} × {debug,release}
./build.ps1 assembleDevDebug        # dev flavor, applicationIdSuffix ".dev"
./build.ps1 assembleProdRelease     # release (minify + resource shrink + proguard)

./build.ps1 clean
./build.ps1 lint                    # lint runs with abortOnError=false, html+xml reports
./build.ps1 test                    # unit tests (JUnit4 + Mockito/Mockito-Kotlin)
./build.ps1 connectedAndroidTest    # instrumented tests (needs device/emulator)

# Single unit test class / method
./build.ps1 testDebugUnitTest --tests "com.hkt.ble.bletools.BluetoothScanFilterTest"
./build.ps1 testDevDebugUnitTest --tests "*.BluetoothScanFilterTest.someMethod"
```

Build flavors are on the `environment` dimension. `dev` adds `BASE_URL=https://dev-api.example.com`, `prod` uses `https://api.example.com` (BuildConfig fields, currently placeholder). APKs land in `app/build/outputs/apk/<flavor>/<buildType>/`.

`local.properties` (not in VCS) must contain `sdk.dir`. The signing key is `key.jks` at repo root (release is currently unsigned unless you wire `signingConfigs` into `app/build.gradle.kts`).

**Note on git:** the working directory is currently **not** a git repository (`git init` has not been run here), so `git-save.ps1` (a `git add -A && git commit` helper) will fail until the repo is initialized.

## Architecture

The app is **single-connection / single-device at a time**, and large amounts of state live in **file-level/global mutable variables** rather than in a repository or ViewModel. Understanding this global-state + polling-thread model is essential before changing protocol or UI code.

### Layer responsibilities

- **`MainActivity.kt`** — BLE scanning, permissions, and connection setup. Uses Nordic's `BluetoothLeScannerCompat` (v18 scanner) in `SCAN_MODE_LOW_LATENCY`. Scanning runs **3 cycles × 10 s with 500 ms gaps** (see the `multi-cycle-scanning` spec). Holds the singleton `BluetoothGatt` in a `companion object` (`getGatt()`/`setGatt()`), so it survives the navigation to `DeviceActivity`. Also owns QR/barcode scanning (ZXing) which auto-connects when a scanned name matches.
- **`BLEUtils.kt`** — BLE plumbing: fixed UUIDs (`BleUuid`), the `BluetoothGattCallback` (`BleCallback`: MTU 512 → discoverServices → enable indication → connected), `BleHelper.sendCommand()`, and `ByteUtils` hex↔byte conversion. Global `connectState` flag.
- **`Communicate.kt`** — **the protocol layer; the most important file.** Defines the wire format, TX frame builder, RX parser, OTA framing, the event state machine, and the parsed-data globals. See below.
- **`DeviceActivity.kt`** — the large device-control screen. Renders device-type-specific config forms, sends commands by setting fields on `mDeviceEvent` (the polling thread dispatches them), and reflects `mDeviceDataString` back into the UI.
- **`DeviceFeatureConfig.kt`** — per-device-type feature flags (e.g. `SYNC_TIMESTAMP` disabled for MPS100 and UDS100).
- **`DebugActivity.kt`** — raw BLE log viewer, toggled by the global `debugActivityPageRun` flag; when active it intercepts `onCharacteristicChanged` instead of the normal parser.
- **`SPUtils.kt`** — SharedPreferences extensions (`getInt`/`putInt`, etc.), file name `"config"`. Used for RSSI threshold (`rssi` key) and scan filters.
- **`ExcelUtils.kt`** + `File.kt` — export device data to `.xls` via Apache POI.

### The wire protocol (`Communicate.kt`)

Frames are exchanged as **hex strings** (data is read/written as bytes under the hood, but all protocol logic works on uppercase hex). Two frame shapes share a `686B74` ("hkt") prefix:

- **App-frame:** `hkt(3) packnum(1) len(2)(cmd+data) cmd(1) data(n) crc(2)` — CRC16-CCITT over `cmd+data`. `len` is the byte length of the following `cmd+data` (e.g. 1 cmd byte + 4 data bytes → `len = 5`).
- **Bootloader/OTA frame:** `hkt(3) len(2)(cmd+...) cmd(1) packnum(2) data(n) crc(2) bootloader(8)` where the suffix is `626F6F746C6F6164` ("bootload").

Key functions:
- `StreamThread` — a background `Thread` started per connection. When `mDeviceEvent.event` is set, it sends the corresponding command; otherwise it **polls device status (cmd `0xFF`) roughly every second** (`timeout >= 2`, 500 ms sleeps). It is the engine that drives config operations.
- `streamDevice(cmd, packNum)` — builds app-frame TX for each command: `0x01` OTA notify, `0xFF` query status, `0xFE` power on/off, `0xFD` calibrate, `0x02` config params (layout differs per device type), `0x03` realtime task, `0x04` timed task, `0x05` delete task, `0x06` sync timestamp.
- `streamRev(content)` — parses RX frames. Walks the payload dispatching on each `cmd` byte (e.g. `0x01` version, `0x03` battery, `0x09`/`0x0A` temp/humidity, `0x10`/`0x11` lat/lon, `0x46` distance, `0x5D–0x60` geomag/radar, `0x86` report period) and writes into `mDeviceData` + `mDeviceDataString`. The `0xFF` ACK also advances the `DeviceEventEnum` state machine (`*_START_EVENT` → `*_FINISH_EVENT`).
- `backBootLoaderBuf(fileSize, fileBytes, cmd, packNum)` — OTA transfer framing; sends firmware in 128-byte (256 hex-char) chunks, pads the final chunk to an 8-byte boundary with `FF`, tracks `otaLevel`/`levelString` progress.

**Signed values** in the protocol are decoded manually via sign-bit checks (`> 0x800000`, `> 0x80000000`, `> 0x8000`) rather than Kotlin's built-in signed types — preserve this pattern when adding fields.

### Adding a new device type or command

This typically requires coordinated edits across several files (this is the recurring cross-cutting task in this repo):
1. `Communicate.kt` — add the `DeviceNameEnum` value (and a `parseDeviceType` branch in `MainActivity`), extend `streamDevice()` TX for the device's config layout, extend `streamRev()` RX parsing for its telemetry bytes, and add any new `DeviceEventEnum` states + their dispatch in `StreamThread` and the `0xFF` ACK handler.
2. `DeviceActivity.kt` — add the device-type-specific UI, reading inputs into `mDeviceEvent` fields and displaying `mDeviceDataString`.
3. `DeviceFeatureConfig.kt` — disable any unsupported features for the new type.
4. `docs/parameter-analysis.md` documents the parameter config/display logic in depth — consult it for field byte layouts.

## Conventions

- **Code style:** 4-space indent, 120-char width, import order Android → AndroidX → `com` → other → kotlin (per `docs/ANDROID_STUDIO_CONFIG.md`). Match existing files.
- **Language:** Code comments are predominantly Chinese; user-facing UI strings are English with a Chinese translation in `app/src/main/res/values-zh/strings.xml`. `MissingTranslation` lint is disabled, so new strings are not blocked if untranslated, but add `values-zh` entries when adding user-facing text.
- **`@SuppressLint("MissingPermission")`** is used pervasively on BLE calls rather than inline permission checks — the runtime permission flow lives in `MainActivity`. Match this when adding BLE calls.
- **Global state first:** New device data/event fields go on the existing `DeviceTypeData` / `DeviceEventData` data classes (and their `*String` display twin), not in new per-Activity state.
- **Concurrency hazard to know:** `StreamThread` (background thread) and `streamRev()` (invoked from BLE callbacks) mutate `mDeviceData` / `mDeviceEvent` / `mDeviceDataString` while the UI thread reads them in `DeviceActivity`. There is no locking. Preserve the existing pattern (UI refreshes off copied snapshots) and be careful adding new cross-thread fields.

## openspec workflow

This repo uses **spec-driven development** via `openspec/` (`schema: spec-driven` in `openspec/config.yaml`):

- Current, approved specs live in `openspec/specs/<capability>/spec.md` (e.g. `bluetooth-scanning`, `multi-cycle-scanning`, `scanning-configuration`, `config-view-caching`, `build-configuration`).
- In-progress or proposed changes live in `openspec/changes/<change-name>/` as `proposal.md`, `design.md`, `tasks.md`, and a `specs/` delta; completed changes are moved to `openspec/changes/archive/`.
- When changing scanning, config-caching, or build behavior, check the matching spec first and treat it as the source of truth for intended behavior.

## Further reading (in `docs/`)

- `ANDROID_STUDIO_CONFIG.md` — full build/lint/ProGuard/flavor reference. Note: parts of it (AGP/Kotlin versions, config-cache) describe a target state that is newer than the actual `build.gradle.kts`; the build files are authoritative for versions.
- `parameter-analysis.md` — byte-level breakdown of the parameter config and telemetry display logic.
- `barcode-scan-deveui-analysis.md` — QR pairing logic analysis.
- `MASTER.md` — design system for the marketing/landing page (web, not the Android UI).
