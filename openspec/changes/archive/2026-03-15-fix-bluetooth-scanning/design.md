## Context

The current `MainActivity.kt` implementation mixes native Android BLE scanning APIs with imports from the Nordic Semiconductor Android BLE Library (`no.nordicsemi.android.support.v18:scanner`). Additionally, a logic error in the device filtering mechanism (`||` instead of `&&`) prevents any devices from being added to the UI list.

## Goals / Non-Goals

**Goals:**
- **Standardize on Nordic Library:** Replace all native `BluetoothLeScanner` and `ScanCallback` usages with their Nordic Compat equivalents.
- **Fix Logic Bug:** Correct the filtering condition in `addDeviceList` to allow valid devices to appear.
- **Clean Code:** Remove unused imports and variable declarations related to the native scanner.
- **Robustness:** Ensure proper handling of permissions and Bluetooth state.

**Non-Goals:**
- Changing the UI layout or design.
- Adding new filtering features (e.g., by RSSI or specific services), other than fixing the existing logic.
- Modifying other parts of the app (e.g., connection logic), though `BleCallback` is involved in connection, we are focusing on scanning.

## Decisions

### 1. Use `BluetoothLeScannerCompat` Singleton
**Decision:** We will use `BluetoothLeScannerCompat.getScanner()` to obtain the scanner instance.
**Rationale:** The Nordic library provides a compat layer that handles various Android version quirks and vendor-specific issues. Using the singleton ensures consistent behavior and resource management.
**Alternative Considered:** Sticking with native `BluetoothAdapter.getDefaultAdapter().getBluetoothLeScanner()`. This was rejected because it is less robust across the fragmented Android ecosystem.

### 2. Strict Type Usage
**Decision:** The `bluetoothLeScanner` variable in `MainActivity` will be explicitly typed as `BluetoothLeScannerCompat` (or inferred as such), and the `scanCallback` will inherit from `no.nordicsemi.android.support.v18.scanner.ScanCallback`.
**Rationale:** Mixing types causes confusion and potential runtime errors if the wrong callback type is passed to the wrong scanner method.

### 3. Logic Correction
**Decision:** Change `if (isScanNullNameDevice || bleDevice.device.name == null)` to `if (isScanNullNameDevice && bleDevice.device.name == null)`.

**Code Contrast:**
```kotlin
// BEFORE (Bugged)
// Filters out EVERYTHING because isScanNullNameDevice is true
if (isScanNullNameDevice || bleDevice.device.name == null) { return }

// AFTER (Fixed)
// Only filters if filtering is enabled AND name is null
if (isScanNullNameDevice && bleDevice.device.name == null) { return }
```

**Rationale:** The intent is to filter out devices *only if* the "filter null names" flag is set AND the device name is null.

## Risks / Trade-offs

- **Dependency on Third-Party Library:** We are increasing reliance on the Nordic library (v1.5.0). However, this library is a standard in the Android BLE community.
- **Callback Signature Mismatch:** We must ensure the `onScanResult` override in `scanCallback` matches the Nordic library's signature exactly.
- **Permissions:** If permissions are denied, the scanner might throw or fail silently. We rely on existing permission checks in `MainActivity` but should add safeguards.
- **Timeout:** Scanning indefinitely consumes battery. We should implement a timeout mechanism in the future (Non-Goal for this fix, but noted).

## Architecture

```
┌─────────────┐     ┌──────────────────────┐     ┌──────────────┐
│ MainActivity │────▶│ BluetoothLeScanner  │────▶│ Device List  │
│   (View)    │     │      Compat         │     │    (UI)      │
└─────────────┘     └──────────────────────┘     └──────────────┘
       │                    │
       │                    ▼
       │            ┌───────────────┐
       └───────────▶│  ScanCallback │
                    │   (Nordic)    │
                    └───────────────┘
```
**Responsibilities:**
- **MainActivity**: UI logic, permission handling, initiating scan.
- **BluetoothLeScannerCompat**: The singleton scanner instance from Nordic library.
- **ScanCallback**: Receives scan results and passes them to UI (via `addDeviceList`).
- **Device List**: RecyclerView displaying `BleDevice` items.
