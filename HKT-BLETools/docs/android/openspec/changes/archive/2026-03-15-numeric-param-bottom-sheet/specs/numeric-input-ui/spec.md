## ADDED Requirements

### Requirement: Numeric Input Bottom Sheet
The system SHALL display a bottom sheet UI for modifying numeric parameters, replacing the previous dialog interface.

#### Scenario: Displaying the Bottom Sheet
- **WHEN** the user taps on a numeric parameter configuration button
- **THEN** a bottom sheet slides up from the bottom of the screen
- **AND** the sheet displays the parameter name, current value, and input field

#### Scenario: Valid Input Submission
- **WHEN** the user enters a valid number within the configured range (e.g., min <= value <= max)
- **AND** taps the "Submit" button
- **THEN** the bottom sheet dismisses
- **AND** the parameter value is updated in the application state
- **AND** the updated value is sent to the device (if applicable)

#### Scenario: Invalid Input Handling
- **WHEN** the user enters a value outside the valid range
- **THEN** the "Submit" button is disabled OR an error message is displayed
- **AND** the system prevents submission

#### Scenario: Cancellation
- **WHEN** the user taps the "Cancel" button or taps outside the bottom sheet
- **THEN** the bottom sheet dismisses
- **AND** no changes are applied to the parameter value
