## 1. Logic Implementation

- [x] 1.1 Update `MainActivity.kt`: Add `scanCycleCount` (Int) member variable.
- [x] 1.2 Update `MainActivity.kt`: Modify `scan()` to accept an optional `isNewSession` parameter (default true).
- [x] 1.3 Update `MainActivity.kt`: In `scan()`, if `isNewSession` is true, reset `scanCycleCount` to 0 and clear the list. If false, keep the list.
- [x] 1.4 Update `MainActivity.kt`: Modify the timeout handler in `scan()` to call a new `onScanCycleComplete()` method instead of just `stopScan()`.
- [x] 1.5 Update `MainActivity.kt`: Implement `onScanCycleComplete()`:
    - Stop current scan.
    - Increment `scanCycleCount`.
    - If `scanCycleCount < 3`: Post a delayed runnable (500ms) to call `scan(isNewSession = false)`.
    - If `scanCycleCount >= 3`: Call `stopScan()` (final stop).
- [x] 1.6 Update `MainActivity.kt`: Update `stopScan()` (manual stop) to clear any pending cycle handlers and reset `scanCycleCount`.

## 2. Verification

- [x] 2.1 Verify compilation: Run `./gradlew assembleDebug`.
- [ ] 2.2 Verify Manual Stop: Start scan, tap Stop immediately. Expect scan to stop and NOT restart.
- [ ] 2.3 Verify 3-Cycle Run: Start scan, wait. Expect scan to run for 10s -> pause -> run 10s -> pause -> run 10s -> stop completely (UI resets).
- [ ] 2.4 Verify List Persistence: During the 3 cycles, verify that the device list is NOT cleared between cycles (devices accumulate).
