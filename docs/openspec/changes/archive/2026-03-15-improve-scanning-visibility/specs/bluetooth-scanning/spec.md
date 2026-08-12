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

### Requirement: Scan Timeout
The system SHALL automatically stop scanning after a fixed duration to conserve battery.

#### Scenario: Automatic Timeout
- **WHEN** scanning starts
- **THEN** the system SHALL automatically stop scanning after 10 seconds
- **AND** update the UI to reflect the stopped state

## MODIFIED Requirements

### Requirement: BLE Device Discovery
The system SHALL scan for nearby Bluetooth Low Energy (BLE) devices using the Nordic Semiconductor Compat Library and display them in a list.

#### Scenario: Scan Start
- **WHEN** the user taps the "Scan" button
- **THEN** the app requests `BLUETOOTH_SCAN` and `BLUETOOTH_CONNECT` permissions (if not granted)
- **THEN** the app starts scanning using `BluetoothLeScannerCompat` with `SCAN_MODE_LOW_LATENCY` and `CALLBACK_TYPE_ALL_MATCHES`
- **THEN** the UI updates to show a scanning indicator

#### Scenario: Device Found
- **WHEN** a BLE device is discovered
- **THEN** the `onScanResult` callback is invoked
- **THEN** the device is filtered based on the current "Filter Null Names" setting (default: false)
- **THEN** if valid, the device is added to the displayed list with its name and RSSI

#### Scenario: Filter Null Names (Configurable)
- **WHEN** the "Filter Null Names" option is ENABLED
- **AND** a device with a null/empty name is discovered
- **THEN** the device is NOT added to the list

#### Scenario: Show Null Names (Default)
- **WHEN** the "Filter Null Names" option is DISABLED (default state)
- **AND** a device with a null/empty name is discovered
- **THEN** the device IS added to the list
- **AND** displayed with a placeholder name (e.g., "N/A" or "Unknown Device")

#### Scenario: Scan Stop
- **WHEN** the user taps the "Stop" button OR selects a device OR the timeout expires
- **THEN** the app stops scanning using `BluetoothLeScannerCompat`
- **THEN** the scanning indicator is hidden

#### Scenario: Permissions Denied
- **WHEN** the user denies Bluetooth permissions
- **THEN** the app SHALL NOT attempt to start scanning
- **THEN** the app should show a toast or dialog explaining the restriction

#### Scenario: Bluetooth Disabled
- **WHEN** the user attempts to scan while Bluetooth is off
- **THEN** the app SHALL prompt the user to enable Bluetooth
- **THEN** the scan SHALL NOT start until Bluetooth is enabled

### Requirement: Timeout
The system SHALL implement an automatic timeout to prevent indefinite scanning.

#### Scenario: Timeout Execution
- **WHEN** a scan has been running for 10 seconds
- **THEN** the system SHALL automatically stop the scan
- **AND** return the UI to the "Scan" (start) state
