## 1. Library Migration

- [x] 1.1 Remove native `android.bluetooth.le.BluetoothLeScanner` import and variable declaration in `app/src/main/java/com/hkt/ble/bletools/MainActivity.kt`.
- [x] 1.2 Change `bluetoothLeScanner` variable type to `no.nordicsemi.android.support.v18.scanner.BluetoothLeScannerCompat` in `MainActivity.kt`.
- [x] 1.3 Initialize `bluetoothLeScanner` using `BluetoothLeScannerCompat.getScanner()` in `MainActivity.kt`.
- [x] 1.4 Update `scanCallback` to inherit from `no.nordicsemi.android.support.v18.scanner.ScanCallback`.
- [x] 1.5 Ensure `onScanResult` and other callback methods match the Nordic library signature exactly.
- [x] 1.6 Verify `no.nordicsemi.android.support.v18:scanner:1.5.0` is used.

## 2. Logic Correction

- [x] 2.1 Locate `addDeviceList` function in `app/src/main/java/com/hkt/ble/bletools/MainActivity.kt`.
- [x] 2.2 Fix the filtering condition from `if (isScanNullNameDevice || bleDevice.device.name == null)` to `if (isScanNullNameDevice && bleDevice.device.name == null)`.

## 3. Tests

- [x] 3.1 Create Unit Test class `app/src/test/java/com/hkt/ble/bletools/BluetoothScanFilterTest.kt`.
- [x] 3.2 Add dependencies: `testImplementation("org.mockito:mockito-core:5.3.1")` and `testImplementation("org.mockito.kotlin:mockito-kotlin:5.0.0")` to `app/build.gradle.kts`.
- [x] 3.3 Implement unit test: `testFilterNullNameDevice_whenEnabled_shouldFilterNullNames` (Verify `||` vs `&&` logic).
- [x] 3.4 Implement unit test: `testFilterNullNameDevice_whenDisabled_shouldAllowNullNames`.
- [x] 3.5 Create Integration Test (UI Test) `app/src/androidTest/java/com/hkt/ble/bletools/ScanActivityTest.kt` (using Espresso).
- [x] 3.6 Implement UI test: Check if "Scan" button exists and is clickable.

## 4. Verification

- [x] 4.1 Verify that the app compiles without errors.
- [x] 4.2 Run unit tests: `./gradlew test` and ensure `BluetoothScanFilterTest` passes.
- [x] 4.3 (Manual) Verify that `bluetoothLeScanner` is not null after initialization by adding a log statement: `Log.d("BluetoothScan", "Scanner initialized: $bluetoothLeScanner")`.
- [ ] 4.4 (Manual) Verify scan works: Run app, ensure Bluetooth is ON, tap Scan. Verify devices appear in the list.
- [ ] 4.5 (Manual) Verify filtering: Check if devices with names appear, and null names (if any) are filtered.
- [ ] 4.6 (Manual) Verify Permissions/BT Off: Deny permissions or turn off BT and ensure app doesn't crash.

## 5. Traceability Matrix

| Requirement | Task IDs |
|---|---|
| REQ-BLE-001 (Scan Start/Library) | 1.1, 1.2, 1.3, 1.6, 4.3 |
| REQ-BLE-002 (Device Found/Callback) | 1.4, 1.5, 4.4 |
| REQ-BLE-003 (Filter Null Names) | 2.1, 2.2, 3.3, 3.4, 4.5 |
| REQ-BLE-004 (Permissions/BT Off) | 4.6 |
| REQ-TEST-001 (Unit Tests) | 3.1, 3.2, 3.3, 3.4, 4.2 |
