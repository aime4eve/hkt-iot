## 1. UI Implementation

- [x] 1.1 Update `dialog_scan_filter.xml`: Add a `Switch` (id: `switch_filter_null_names`) for toggling "Filter devices without names".
- [x] 1.2 Update `strings.xml`: Add string resources for the new switch label (e.g., `filter_null_names`).

## 2. Logic Implementation

- [x] 2.1 Update `MainActivity.kt`: Change `isScanNullNameDevice` default value to `false`.
- [x] 2.2 Update `MainActivity.kt`: In `showScanFilterDialog`, bind the new Switch to `isScanNullNameDevice` state and persist changes.
- [x] 2.3 Update `MainActivity.kt`: Modify `scan()` method to use `ScanSettings.Builder`.
- [x] 2.4 Update `MainActivity.kt`: Set `scanMode` to `SCAN_MODE_LOW_LATENCY`.
- [x] 2.5 Update `MainActivity.kt`: Set `callbackType` to `CALLBACK_TYPE_ALL_MATCHES`.
- [x] 2.6 Update `MainActivity.kt`: Implement `stopScan` timeout logic using `Handler.postDelayed(..., 10000)`.
- [x] 2.7 Update `MainActivity.kt`: Ensure `stopScan()` cancels the timeout handler to avoid leaks or double-stops.

## 3. Verification

- [x] 3.1 Verify compilation: Run `./gradlew assembleDebug` to ensure no build errors.
- [ ] 3.2 Verify Default Behavior: Run app, start scan. Expect to see devices with null names (if any) and scan stops after 10s.
- [ ] 3.3 Verify Filter Toggle: Open filter dialog, enable "Filter devices without names", start scan. Expect null-name devices to be hidden.
- [ ] 3.4 Verify Timeout: Start scan, wait 10s. Expect scan to stop automatically and UI to update (button changes to "Start").
