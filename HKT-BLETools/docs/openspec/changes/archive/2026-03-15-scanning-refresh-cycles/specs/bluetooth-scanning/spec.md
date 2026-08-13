## ADDED Requirements

### Requirement: Multi-Cycle Scanning
The system SHALL execute 3 consecutive scan cycles when a scan session is initiated.

#### Scenario: Cycle Execution
- **WHEN** the user starts a scan
- **THEN** the system SHALL run 3 consecutive scan cycles
- **AND** each cycle SHALL last for the defined timeout duration (10s)
- **AND** there SHALL be a brief pause between cycles to allow the Bluetooth stack to reset

### Requirement: Incremental Results
The system SHALL accumulate scan results across all cycles within a single session.

#### Scenario: Result Accumulation
- **WHEN** a new scan cycle starts within the same session (Cycle 2 or 3)
- **THEN** the existing list of discovered devices SHALL NOT be cleared
- **AND** newly discovered devices SHALL be added to the existing list

## MODIFIED Requirements

### Requirement: Timeout
The system SHALL implement an automatic timeout to prevent indefinite scanning.

#### Scenario: Timeout Execution (Single Cycle)
- **WHEN** a single scan cycle has been running for 10 seconds
- **THEN** the system SHALL stop the current scan
- **AND** if fewer than 3 cycles have run, start the next cycle
- **AND** if 3 cycles have run, stop the session completely

### Requirement: BLE Device Discovery
The system SHALL scan for nearby Bluetooth Low Energy (BLE) devices using the Nordic Semiconductor Compat Library and display them in a list.

#### Scenario: Scan Start (New Session)
- **WHEN** the user taps the "Scan" button
- **THEN** the device list is cleared
- **THEN** the app starts the first scan cycle (Cycle 1 of 3)
- **THEN** the UI updates to show a scanning indicator

#### Scenario: Scan Stop (Manual)
- **WHEN** the user taps the "Stop" button
- **THEN** the app stops the current scan immediately
- **THEN** any pending future cycles are cancelled
- **THEN** the scanning indicator is hidden

#### Scenario: Scan Stop (Automatic Completion)
- **WHEN** the 3rd scan cycle completes
- **THEN** the app stops scanning
- **THEN** the scanning indicator is hidden
- **THEN** the "Scan" button returns to its initial state
