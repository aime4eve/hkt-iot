## ADDED Requirements

### Requirement: Parameter Configuration Flow
The device configuration flow SHALL utilize the `numeric-input-ui` for all numeric parameter modifications.

#### Scenario: UDS100 Threshold Configuration
- **WHEN** the user configures UDS100 High/Low Thresholds
- **THEN** the `numeric-input-ui` bottom sheet is displayed instead of the `dialog_config_uds100` layout's EditText
- **AND** the input range is validated against 30-4500 (mm)

#### Scenario: UDS100 Report Period Configuration
- **WHEN** the user configures UDS100 Report Period
- **THEN** the `numeric-input-ui` bottom sheet is displayed
- **AND** the input range is validated against 1-1440 (minutes)

#### Scenario: UDS100 GPS Period Configuration
- **WHEN** the user configures UDS100 GPS Period
- **THEN** the `numeric-input-ui` bottom sheet is displayed
- **AND** the input range is validated against 10-1440 (minutes) or 0 (off)

#### Scenario: DC200 Pulse Configuration
- **WHEN** the user configures DC200 Pulse Timed/Realtime
- **THEN** the `numeric-input-ui` bottom sheet is displayed
- **AND** the input range is validated against 0-65535
