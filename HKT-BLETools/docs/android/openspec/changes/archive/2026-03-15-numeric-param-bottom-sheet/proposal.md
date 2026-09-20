## Why

The current numeric parameter modification interface (using dialogs or inline edits) can be improved for better usability. A bottom sheet interface that slides up from the bottom provides a more modern and focused interaction, allowing users to clearly review their changes before committing or discarding them.

## What Changes

- Replace the existing numeric parameter modification interaction with a bottom sheet UI.
- Implement a slide-up panel that appears when a numeric parameter is tapped.
- The panel will display the parameter name, current value, and an input field.
- Add "Submit" and "Cancel" buttons to the panel for explicit confirmation.
- Ensure the input validation logic (e.g., range checks) is preserved in the new UI.

## Capabilities

### New Capabilities
- `numeric-input-ui`: A reusable bottom sheet component for modifying numeric values with validation and confirmation.

### Modified Capabilities
- `device-configuration`: Update the device configuration flow to use the new numeric input UI instead of existing dialogs.

## Impact

- **UI/UX**: Significant change to how users interact with numeric settings.
- **Code**:
  - `DeviceActivity.kt`: Will need to be updated to launch the bottom sheet instead of dialogs.
  - New Layout/Fragment/Dialog for the bottom sheet.
  - `Communicate.kt`: Interaction logic remains, but UI trigger points change.
