## Why

The current Bluetooth scanning implementation has significant limitations that affect usability and reliability:
1.  **Slow Discovery**: The default scan mode is not optimized for quick discovery, leading to long wait times.
2.  **Incomplete Results**: A default filtering logic (`isScanNullNameDevice = true`) hides many valid BLE devices that don't broadcast a name initially, without giving the user a way to disable this filter.
3.  **Potential Resource Drain**: Scanning runs indefinitely without a timeout, which can consume excessive battery and system resources.

Improving these aspects is critical for a professional-grade BLE tool, ensuring users can find all available devices quickly and reliably.

## What Changes

- **Performance Tuning**: Switch to `SCAN_MODE_LOW_LATENCY` for faster device discovery and implement a 10-second scan timeout.
- **Logic Update**: Change the default behavior to show all devices (including those with null names) and ensure `CALLBACK_TYPE_ALL_MATCHES` is used to catch all advertisements.
- **UI Enhancement**: Add a "Filter Null Names" switch to the existing Scan Filter dialog, empowering users to control list clutter.
- **State Feedback**: Provide better visual feedback during the scanning process (implied by timeout handling).

## Capabilities

### New Capabilities
<!-- Capabilities being introduced. Replace <name> with kebab-case identifier (e.g., user-auth, data-export, api-rate-limiting). Each creates specs/<name>/spec.md -->
- `scanning-configuration`: Defines how scanning parameters (mode, timeout, filters) are configured and exposed to the user.

### Modified Capabilities
<!-- Existing capabilities whose REQUIREMENTS are changing (not just implementation).
     Only list here if spec-level behavior changes. Each needs a delta spec file.
     Use existing spec names from openspec/specs/. Leave empty if no requirement changes. 
-->
- `bluetooth-scanning`: Updating requirements to support low-latency mode, timeout, and optional null-name filtering.

## Impact

- **Affected Code**: `MainActivity.kt` (scan logic, filter logic), `dialog_scan_filter.xml` (UI layout).
- **User Experience**: Users will see devices faster and have more control over which devices are displayed.
- **System**: Battery usage for scanning will be more predictable due to the timeout.
