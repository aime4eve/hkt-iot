## Context

The current `MainActivity.kt` uses a basic `startScan()` call with no settings, defaulting to balanced power mode and potentially aggressive filtering. The `isScanNullNameDevice` flag is hardcoded to `true` (meaning "filter out null names"), which is counter-intuitive and hides many devices. The UI provides an RSSI filter but no way to toggle name filtering.

## Goals / Non-Goals

**Goals:**
- **Maximize Discovery Speed:** Use `SCAN_MODE_LOW_LATENCY` to find devices as quickly as possible.
- **Maximize Discovery Completeness:** Capture all advertisement packets (`CALLBACK_TYPE_ALL_MATCHES`) and default to showing all devices.
- **User Control:** Expose the null-name filtering option in the UI.
- **Resource Safety:** Implement a 10-second timeout to prevent battery drain.

**Non-Goals:**
- Implementing background scanning services or complex scan scheduling.
- changing the data model for `BleDevice`.
- Refactoring the entire `MainActivity` (scope is limited to scanning logic).

## Decisions

### 1. Scan Settings Configuration
**Decision:** Use `ScanSettings.Builder` to explicitly set mode and callback type.
**Rationale:** Default settings are often optimized for power, not performance. For a "Scanner" tool, performance (finding devices) is the priority during the active scan window.
- `setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)`: Prioritizes speed.
- `setCallbackType(ScanSettings.CALLBACK_TYPE_ALL_MATCHES)`: Ensures we get updates even if signal strength changes or multiple packets are received.

### 2. Timeout Mechanism
**Decision:** Use `Handler.postDelayed` to stop scanning after 10 seconds.
**Rationale:** `SCAN_MODE_LOW_LATENCY` consumes significant power. A hard timeout protects the user's battery if they forget to stop scanning. 10 seconds is a standard duration for discovery.

### 3. UI for Filtering
**Decision:** Add a `Switch` to `dialog_scan_filter.xml` for "Filter devices without names".
**Rationale:** This places the control contextually with the existing RSSI filter. It's a simple boolean toggle that maps directly to the `isScanNullNameDevice` logic.
**Migration:** The existing variable `isScanNullNameDevice` will be controlled by this switch instead of being hardcoded/static.

## Risks / Trade-offs

- **Battery Usage:** `LOW_LATENCY` scanning uses more power.
  - *Mitigation:* The 10-second timeout strictly limits the duration of high-power usage.
- **List Clutter:** Showing null-name devices by default will populate the list with many "N/A" entries (Mac addresses only).
  - *Mitigation:* The user can easily toggle the filter back on if the list is too noisy. The default is "show everything" to ensure "it works" first.
