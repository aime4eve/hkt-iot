## ADDED Requirements

### Requirement: Scan Mode Configuration
The system SHALL use low-latency scanning mode to maximize device discovery speed.

#### Scenario: Low Latency Mode
- **WHEN** the user starts a scan
- **THEN** the system SHALL configure the scanner with `SCAN_MODE_LOW_LATENCY`

### Requirement: Scan Callback Type
The system SHALL configure the scanner to report all advertisement matches to ensure no devices are missed due to signal fluctuations.

#### Scenario: All Matches Callback
- **WHEN** the user starts a scan
- **THEN** the system SHALL configure the scanner with `CALLBACK_TYPE_ALL_MATCHES`

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
- **THEN** the RSSI threshold is read from settings and cached for the session
- **THEN** the app requests `BLUETOOTH_SCAN` and `BLUETOOTH_CONNECT` permissions (if not granted)
- **THEN** the app starts the first scan cycle (Cycle 1 of 3) using `BluetoothLeScannerCompat` with `SCAN_MODE_LOW_LATENCY` and `CALLBACK_TYPE_ALL_MATCHES`
- **THEN** the UI updates to show a scanning indicator

#### Scenario: Device Found
- **WHEN** a BLE device is discovered
- **THEN** the `onScanResult` callback is invoked
- **THEN** the device is filtered based on the current "Filter Null Names" setting (default: false) and the current RSSI threshold
- **THEN** if the device is new, it is added to the displayed list with its name and RSSI
- **THEN** if the device already exists in the list, its RSSI value (and name if previously null) SHALL be updated

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
