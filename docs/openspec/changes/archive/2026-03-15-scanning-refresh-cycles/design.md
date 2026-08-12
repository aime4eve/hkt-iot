## Context

Currently, the `scan()` function initiates a single scan session that automatically stops after 10 seconds (as implemented in `improve-scanning-visibility`). The user must manually tap "Scan" again to refresh the list. To improve discovery, we want to automate this process to run 3 consecutive cycles.

## Goals / Non-Goals

**Goals:**
- **Automated Refresh:** Run 3 back-to-back scan cycles when "Scan" is tapped.
- **Incremental List:** Do not clear the device list between cycles within the same session.
- **Clear State:** The UI should indicate scanning is active throughout the 3 cycles and only reset to "Start" state after the 3rd cycle completes.

**Non-Goals:**
- Changing the 10-second duration per cycle (we will keep this as the base unit).
- Implementing complex background scheduling (cycles run only while the activity is foreground/active).

## Decisions

### 1. Cycle Management Logic
**Decision:** Introduce a `scanCycleCount` variable in `MainActivity` to track the current cycle (0 to 3).
**Rationale:** Simple integer tracking is sufficient.
- When `scan()` is called:
  - If `scanCycleCount == 0`: Clear list (new session).
  - Start scan.
  - Post delayed runnable for 10s.
- When 10s timeout hits:
  - Stop current scan.
  - Increment `scanCycleCount`.
  - If `scanCycleCount < 3`:
    - Wait a short delay (e.g., 500ms) to allow BT stack to settle.
    - Call `scan()` again (without clearing list).
  - If `scanCycleCount >= 3`:
    - Reset `scanCycleCount = 0`.
    - Update UI to "Stopped" state.

### 2. List Clearing Strategy
**Decision:** Only clear `mList` and `addressList` when `scanCycleCount == 0`.
**Rationale:** This fulfills the requirement of "incrementally adding" devices during the 3-cycle session.

### 3. Stopping Logic
**Decision:** If the user manually taps "Stop", the entire session is cancelled (`scanCycleCount` reset to 0, all pending handlers removed).
**Rationale:** User intent to stop overrides the automated cycle plan.

## Risks / Trade-offs

- **User Confusion:** Users might wonder why scanning is restarting.
  - *Mitigation:* A Toast or UI indicator saying "Starting cycle X/3" could help, but for now we'll keep it simple as "Scanning...".
- **Battery:** 30 seconds of scanning is significant.
  - *Mitigation:* It's still a user-initiated action, and manual stop is available.

## Architecture

No major architectural changes. Logic is contained within `MainActivity.kt`.
- `scan()`: Modified to accept a `isNewSession` or infer from state.
- `stopScan()`: Handles intermediate stops vs final stop.
