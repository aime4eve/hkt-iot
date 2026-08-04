## ADDED Requirements

### Requirement: BLE Device Discovery
The system SHALL scan for nearby Bluetooth Low Energy (BLE) devices using the Nordic Semiconductor Compat Library and display them in a list.

#### Scenario: Scan Start
- **WHEN** the user taps the "Scan" button
- **THEN** the app requests `BLUETOOTH_SCAN` and `BLUETOOTH_CONNECT` permissions (if not granted)
- **THEN** the app starts scanning using `BluetoothLeScannerCompat`
- **THEN** the UI updates to show a scanning indicator

#### Scenario: Device Found
- **WHEN** a BLE device is discovered
- **THEN** the `onScanResult` callback is invoked
- **THEN** the device is filtered based on user preferences (e.g., non-null name)
- **THEN** if valid, the device is added to the displayed list with its name and RSSI

#### Scenario: Filter Null Names (Logic Fix)
- **WHEN** the "Filter Null Names" option is enabled (default: true)
- **AND** a device with a null/empty name is discovered
- **THEN** the device is NOT added to the list
- **BUT** if the device HAS a name, it SHALL be added to the list (Fixing the bug where these were also filtered)

#### Scenario: Scan Stop
- **WHEN** the user taps the "Stop" button OR selects a device
- **THEN** the app stops scanning using `BluetoothLeScannerCompat`
- **THEN** the scanning indicator is hidden

#### Scenario: Permissions Denied
- **WHEN** the user denies Bluetooth permissions
- **THEN** the app SHALL NOT attempt to start scanning
- **THEN** the app should show a toast or dialog explaining the restriction (Relies on existing logic)

#### Scenario: Bluetooth Disabled
- **WHEN** the user attempts to scan while Bluetooth is off
- **THEN** the app SHALL prompt the user to enable Bluetooth
- **THEN** the scan SHALL NOT start until Bluetooth is enabled

### Non-Functional Requirements

#### Requirement: Compatibility
- The scanner SHALL function correctly on Android 8.0 (API 26) and above (based on `minSdk` in `build.gradle.kts`).
- The scanner SHALL use `no.nordicsemi.android.support.v18:scanner:1.5.0`.

#### Requirement: Permissions
- The app SHALL handle `BLUETOOTH_SCAN` (Android 12+) and Location permissions (Android 6-11) appropriately before scanning.

#### Requirement: Timeout
- **Note**: Current implementation relies on manual stop. A future improvement should add an automatic timeout to conserve battery.
