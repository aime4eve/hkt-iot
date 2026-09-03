# iOS

This directory is the source root for the native SwiftUI iOS app. `HKTBLETools.xcodeproj` will live beside the source directories; do not add another application-named directory under `ios/`.

Planned layout:

```text
ios/
  HKTBLETools.xcodeproj
  App/
  Core/
    BLE/
    Protocol/
    OTA/
    Device/
  Features/
    Scan/
    Device/
    OTA/
    Debug/
    Settings/
  DesignSystem/
  Support/
  Resources/
  Tests/
```

The first milestone covers scanning, connection, basic status display, firmware selection, and OTA for UDS100, SVC100, EPS100, and MPS100. The protocol layer must model the Android capability surface so later configuration and diagnostic features do not require structural rework.
