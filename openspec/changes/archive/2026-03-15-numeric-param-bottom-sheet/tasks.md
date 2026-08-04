## 1. Setup & UI Component

- [x] 1.1 Create layout resource `dialog_numeric_input.xml` for the bottom sheet.
- [x] 1.2 Create `NumericInputBottomSheet` class extending `BottomSheetDialogFragment`.
- [x] 1.3 Implement initialization logic (title, value, range) and validation in `NumericInputBottomSheet`.
- [x] 1.4 Implement callback interface `OnNumericInputListener` for returning results.

## 2. Integration in DeviceActivity

- [x] 2.1 Update `DeviceActivity.kt` to implement `OnNumericInputListener` or handle callbacks.
- [x] 2.2 Modify UDS100 configuration logic: Replace `EditText` in `dialog_config_uds100.xml` with read-only text or buttons that trigger the bottom sheet.
- [x] 2.3 Modify DC200 configuration logic: Update pulse/period inputs to trigger the bottom sheet.
- [x] 2.4 Ensure all numeric parameters (Thresholds, Periods, Pulse, etc.) use the new bottom sheet.

## 3. Verification

- [x] 3.1 Verify UDS100 parameters (Thresholds, Report Period, GPS Period) open the sheet and update values correctly.
- [x] 3.2 Verify DC200 parameters (Pulse, Buffeting) open the sheet and update values correctly.
- [x] 3.3 Verify input validation prevents invalid values.
- [x] 3.4 Verify cancel action discards changes.
