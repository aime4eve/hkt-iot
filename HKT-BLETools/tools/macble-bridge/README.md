# macble-bridge — M7 真机验证桥（Mac 蓝牙 ↔ Android v2 协议栈）

模拟器无蓝牙硬件（AOSP 模拟器不透传宿主机 BT），M7 真机验证走**桥路线**：
Mac 的 CoreBluetooth 连接旁边的真机（MPS/EPS/UDS…），经 TCP 行 JSON 把
`BluetoothPort`/`PeripheralLink` 语义透传给跑在桌面 JVM 的安卓 v2 纯 Kotlin 协议栈
（`android/harness` → core-protocol / core-ble / core-ota，零 Android 依赖）。
真机与 App 走**同一套**会话/OTA 代码路径。

```
harness(JVM: DeviceSession/OtaEngine) ⇄ TCP 9876 ⇄ MacBLEBridge.app(CoreBluetooth) ⇄ BLE ⇄ 真机
```

## 一次性准备

1. 构建桥：`cd tools/macble-bridge && swift build`
2. 同步进 .app 壳（裸 CLI 拿不到蓝牙授权，必须经 GUI 会话启动）：
   `cp .build/debug/macble-bridge MacBLEBridge.app/Contents/MacOS/macble-bridge`
   （`.app` 的 Info.plist 含 `NSBluetoothAlwaysUsageDescription`；
   SwiftPM 侧另经 `__TEXT,__info_plist` 链接段内嵌，双保险）
3. 构建 harness：`cd android && ./gradlew :harness:installDist`（JAVA_HOME 见 AGENTS.md §1b）
4. **蓝牙授权**（一次性）：系统设置 → 隐私与安全性 → 蓝牙 → 添加/启用 `MacBLEBridge`。
   ⚠️ 首次 TCC 请求若被拒绝（弹窗点错/后台自动拒），之后不再弹窗、扫描静默无结果——
   必须到设置里手工放行。可用 `{"cmd":"state"}` 探测：`{"state":5,"auth":3}` 为就绪
   （state 5=poweredOn；auth 3=allowed，2=denied）。

## 跑验证

```bash
open --stderr /tmp/bridge.err tools/macble-bridge/MacBLEBridge.app   # 常驻桥（TCP 9876）
cd android
./harness/build/install/harness/bin/harness scan                     # 扫描入列设备
./harness/build/install/harness/bin/harness verify MPS               # 连接→轮询真值→0x02 同值回写→UDS 对时
./harness/build/install/harness/bin/harness verify UDS
./harness/build/install/harness/bin/harness ota MPS <固件.bin>        # 三重防线→真包传输→重连读版本
```

真包在 `HKT-Firmwares/in-house/LoRaWAN_ParkingSensor/USER/Firmware/`（MPS100_EU868_V11.28 等）。

## 无蓝牙冒烟测试

`fake_device_bridge.py` 在同一端口模拟"桥+设备"（夹具帧/ACK/bootloader 应答），
可在无蓝牙权限/无真机时全链路验证 harness：

```bash
python3 tools/macble-bridge/fake_device_bridge.py &   # 占 9876（先停真桥）
./harness/build/install/harness/bin/harness verify MPS
```

## 桥协议（行 JSON）

→ `{"cmd":"ping"|"state"|"scan"|"scanStop"|"connect","id":..|"disconnect"|"write","hex":..}`
← `{"ev":"pong"|"state","state":..,"auth":..|"scan","name":..,"id":..,"rssi":..|"ready"|"connectFailed","reason":..|"rx","hex":..|"disc"}`

ACK 帧（bootloader）= `hkt(3) cmd(1) count(2 BE) "bootload"(8)` 共 14 字节（无 len 无 CRC）。
