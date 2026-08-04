## Why

Single-pass scanning often misses devices due to signal interference or advertising intervals, leading to user frustration. A multi-cycle scanning approach that refreshes automatically provides a more robust and user-friendly discovery experience.

## What Changes

- **Multi-Cycle Scanning**: The scan process will now default to running 3 consecutive refresh cycles (start -> wait -> stop -> restart -> ...).
- **Incremental Results**: Devices found in each cycle will be added to the existing list, ensuring a comprehensive view of the environment.
- **UI Feedback**: The "Scan" button will only reset to its initial state after all 3 cycles are complete, indicating the full discovery process is finished.

## Capabilities

### New Capabilities
<!-- Capabilities being introduced. Replace <name> with kebab-case identifier (e.g., user-auth, data-export, api-rate-limiting). Each creates specs/<name>/spec.md -->
- `multi-cycle-scanning`: Defines the logic for sequential scan cycles and result accumulation.

### Modified Capabilities
<!-- Existing capabilities whose REQUIREMENTS are changing (not just implementation).
     Only list here if spec-level behavior changes. Each needs a delta spec file.
     Use existing spec names from openspec/specs/. Leave empty if no requirement changes. 
-->
- `bluetooth-scanning`: Updating the core scanning behavior to support multiple cycles instead of a single pass.

## Impact

- **Affected Code**: `MainActivity.kt` (scan control logic, timeout handling).
- **User Experience**: Users will see the list populate over a longer period (3 cycles), resulting in more discovered devices.
- **Performance**: Scanning duration will increase (e.g., 3 x 10s = 30s total), but with higher reliability.
