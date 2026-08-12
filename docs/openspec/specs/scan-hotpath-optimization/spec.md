## ADDED Requirements

### Requirement: O(1) Address Lookup
The system SHALL use a hash-based data structure for device address deduplication, ensuring constant-time lookup regardless of the number of discovered devices.

#### Scenario: Address deduplication performance
- **WHEN** a scan result is received for a device address
- **THEN** the system SHALL determine whether the address has been seen before in O(1) time

### Requirement: RSSI Real-Time Update
The system SHALL update the RSSI value of an already-discovered device when a new advertisement is received from the same address.

#### Scenario: RSSI update on subsequent advertisement
- **WHEN** a BLE advertisement is received from a device already in the list
- **THEN** the system SHALL update the stored RSSI value with the new reading

#### Scenario: Device name late resolution
- **WHEN** a BLE advertisement is received from a device already in the list
- **AND** the stored name is "N/A" but the new advertisement contains a non-null name
- **THEN** the system SHALL update the stored name with the new value

#### Scenario: RSSI filter applied to updated value
- **WHEN** a device's RSSI is updated to a value below the current RSSI threshold
- **THEN** the device SHALL remain in the list (filtering only applies at add time; removal happens when filter conditions change)

### Requirement: UI Update Debouncing
The system SHALL debounce RecyclerView updates to avoid excessive UI refreshes during high-frequency scan callbacks.

#### Scenario: Debounced refresh timing
- **WHEN** one or more scan results are received within a 300ms window
- **THEN** the system SHALL perform exactly one RecyclerView update at the end of that window

#### Scenario: First device appears promptly
- **WHEN** the first device is discovered in a scan session
- **THEN** the RecyclerView update SHALL occur within at most 300ms

### Requirement: Efficient Filter Application
The system SHALL NOT execute full-list filtering on every scan callback. Full-list filtering SHALL only occur when filter conditions change.

#### Scenario: Filter not called on each callback
- **WHEN** a scan result is received
- **THEN** the system SHALL check the new device against current filter criteria without iterating the entire device list

#### Scenario: Filter reapplied on condition change
- **WHEN** the user changes the RSSI threshold or toggles the null-name filter
- **THEN** the system SHALL reapply the filter to the entire device list once
- **AND** remove devices that no longer meet the criteria
