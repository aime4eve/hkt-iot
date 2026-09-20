## ADDED Requirements

### Requirement: Cycle Management
The system SHALL manage the state of the 3-cycle scan process, including starting, stopping, and transitioning between cycles.

#### Scenario: Cycle Transition
- **WHEN** a scan cycle (1 or 2) completes
- **THEN** the system SHALL stop the current scan
- **AND** wait for a short delay (e.g., 500ms)
- **AND** start the next scan cycle
- **AND** DO NOT clear the device list

#### Scenario: Cycle Reset
- **WHEN** a scan session completes (after 3 cycles) OR is manually stopped
- **THEN** the cycle counter SHALL be reset to 0
- **AND** the next scan initiated by the user SHALL be treated as a new session (clearing the list)
