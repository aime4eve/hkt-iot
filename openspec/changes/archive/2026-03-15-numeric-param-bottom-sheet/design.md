## Context

The application currently uses standard Android Dialogs for configuring device parameters. To improve the user experience, we are transitioning to a bottom sheet interaction model for numeric parameter modifications. This provides a more modern feel and better one-handed usage.

## Goals / Non-Goals

**Goals:**
- Create a reusable `NumericInputBottomSheet` component.
- Replace existing numeric configuration dialogs in `DeviceActivity` with this new component.
- Ensure input validation (min/max range) is enforced before submission.
- Provide clear "Submit" and "Cancel" actions.

**Non-Goals:**
- Redesigning the entire application UI.
- Changing the underlying data communication logic (`Communicate.kt`).
- modifying non-numeric parameters at this stage.

## Decisions

### UI Component
We will use `BottomSheetDialogFragment` from the Android Material Components library (or standard support library if Material is not available, though `BottomSheetDialog` is preferred).

### Data Flow
1.  **Trigger**: User clicks a configuration button in `DeviceActivity`.
2.  **Launch**: `DeviceActivity` instantiates `NumericInputBottomSheet`, passing:
    -   Title (Parameter Name)
    -   Current Value
    -   Min Value (for validation)
    -   Max Value (for validation)
    -   (Optional) Unit
3.  **Interaction**: User inputs a value. The "Submit" button is enabled only if the value is valid.
4.  **Result**: On "Submit", the bottom sheet dismisses and returns the value to `DeviceActivity` via a callback or interface. `DeviceActivity` then updates `mDeviceEvent` and handles the BLE command sending.

### Keyboard Handling
The bottom sheet must handle the soft keyboard gracefully. We will ensure the input field is not obscured by the keyboard.

## Risks / Trade-offs

**Risk**: Soft Keyboard covering input.
**Mitigation**: Use `WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE` in the dialog's window or ensure the bottom sheet is expanded when the keyboard opens.

**Risk**: Complex validation logic beyond simple range.
**Mitigation**: For now, simple min/max range validation covers most use cases (thresholds, periods). If complex logic is needed, we can pass a validation strategy or lambda.
