## Why

The current Bluetooth scanning implementation has two critical issues:
1.  **Logic Bug**: A boolean logic error in `MainActivity.kt` (`||` instead of `&&`) causes all scanned devices to be filtered out, resulting in an empty device list.
2.  **Library Confusion**: The codebase imports the Nordic Bluetooth LE Scanner Compat library but instantiates and uses the native Android `BluetoothLeScanner`. This inconsistency can lead to compatibility issues across different Android versions and device manufacturers.

Fixing these issues is necessary to restore basic Bluetooth scanning functionality and ensure robust performance across the Android ecosystem.

## What Changes

- **Logic Fix**: Correct the filtering condition in `addDeviceList` to properly display scanned devices.
- **Library Migration**: Fully migrate to `BluetoothLeScannerCompat` (Nordic library) for scanner instantiation and callbacks.
- **Cleanup**: Remove unused imports and references to the native `android.bluetooth.le` scanner classes.

## Capabilities

### New Capabilities
<!-- Capabilities being introduced. Replace <name> with kebab-case identifier (e.g., user-auth, data-export, api-rate-limiting). Each creates specs/<name>/spec.md -->
- `bluetooth-scanning`: Core capability for scanning and listing BLE devices.

### Modified Capabilities
<!-- Existing capabilities whose REQUIREMENTS are changing (not just implementation).
     Only list here if spec-level behavior changes. Each needs a delta spec file.
     Use existing spec names from openspec/specs/. Leave empty if no requirement changes. 
-->

## Impact

- **Affected Code**: `app/src/main/java/com/hkt/ble/bletools/MainActivity.kt`
- **Dependencies**: Relies on `no.nordicsemi.android.support.v18:scanner:1.5.0` (already in project).
- **Behavior**: The app will now correctly display discovered BLE devices, and scanning will be more reliable on older/fragmented Android versions.
