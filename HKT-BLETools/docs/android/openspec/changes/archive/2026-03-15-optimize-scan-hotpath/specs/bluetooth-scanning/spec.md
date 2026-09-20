## MODIFIED Requirements

### Requirement: BLE Device Discovery
The system SHALL scan for nearby Bluetooth Low Energy (BLE) devices using the Nordic Semiconductor Compat Library and display them in a list.

#### Scenario: Device Found
- **WHEN** a BLE device is discovered
- **THEN** the `onScanResult` callback is invoked
- **THEN** the device is filtered based on the current "Filter Null Names" setting (default: false) and the current RSSI threshold
- **THEN** if the device is new, it is added to the displayed list with its name and RSSI
- **THEN** if the device already exists in the list, its RSSI value (and name if previously null) SHALL be updated

#### Scenario: Scan Start (New Session)
- **WHEN** the user taps the "Scan" button
- **THEN** the device list is cleared
- **THEN** the RSSI threshold is read from settings and cached for the session
- **THEN** the app starts the first scan cycle (Cycle 1 of 3)
- **THEN** the UI updates to show a scanning indicator

#### Scenario: Filter Null Names (Configurable)
- **WHEN** the "Filter Null Names" option is ENABLED
- **AND** a device with a null/empty name is discovered
- **THEN** the device is NOT added to the list

#### Scenario: Show Null Names (Default)
- **WHEN** the "Filter Null Names" option is DISABLED (default state)
- **AND** a device with a null/empty name is discovered
- **THEN** the device IS added to the list
- **AND** displayed with a placeholder name (e.g., "N/A" or "Unknown Device")

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

#### Scenario: Permissions Denied
- **WHEN** the user denies Bluetooth permissions
- **THEN** the app SHALL NOT attempt to start scanning
- **THEN** the app should show a toast or dialog explaining the restriction

#### Scenario: Bluetooth Disabled
- **WHEN** the user attempts to scan while Bluetooth is off
- **THEN** the app SHALL prompt the user to enable Bluetooth
- **THEN** the scan SHALL NOT start until Bluetooth is enabled
