## ADDED Requirements

### Requirement: Filter Configuration UI
The system SHALL provide a user interface to configure scanning filters, including RSSI and device name filtering.

#### Scenario: Open Filter Dialog
- **WHEN** the user taps the filter icon
- **THEN** a bottom sheet dialog appears with filter options

#### Scenario: Toggle Null Name Filter
- **WHEN** the user toggles the "Filter devices without names" switch
- **THEN** the preference is saved
- **AND** subsequent scans will respect this filter setting

#### Scenario: Default Filter State
- **WHEN** the app is first installed
- **THEN** the "Filter devices without names" switch SHALL be disabled (OFF) by default

### Requirement: Scan Timeout Feedback
The system SHALL visually indicate when scanning is active and when it stops due to timeout.

#### Scenario: Timeout Completion
- **WHEN** the scan times out (10s)
- **THEN** the "Stop" button/icon changes back to "Start"
- **AND** the scanning animation/progress indicator stops
