# HKT BLETools

HKT BLETools 是一款 Android/Kotlin 应用，用于扫描、连接、配置和升级 HKT 系列 BLE IoT 设备。应用通过自定义 `hkt` 二进制协议与设备通信，当前配置版本为 `3.17 (versionCode 20260827)`。

## 支持的设备

| 设备类型 | 说明 | 名称匹配 |
| --- | --- | --- |
| UDS100 | 超声波垃圾桶满溢监测传感器 | `UDS100` |
| DC200 系列 | 地磁/雷达车位传感器 | `DC200`、`EPS100`、`MPS100` |
| SVC100 | 电磁阀控制器 | `SVC100` |

`MainActivity.parseDeviceType()` 将 DC200、EPS100 和 MPS100 统一解析为 `NAME_DC200`，三类设备共用配置界面和地磁/雷达数据处理逻辑。

## 当前功能

### 扫描与连接

- BLE 低延迟扫描：每轮 10 秒，共 3 轮，轮间间隔 500 毫秒。
- 扫描过滤：按 RSSI 阈值过滤，并可选择过滤无名称设备。
- 二维码/条码连接：基于 ZXing 扫描设备标识并进入目标扫描。
- 手工录入 EUI：输入 16 位十六进制 DevEUI，默认前缀 `0095690`，按 EUI 后 6 位匹配目标设备。
- 设备列表使用 DiffUtil 更新，减少重复刷新。

### 设备配置

每种设备类型提供专属配置界面：

- UDS100：满溢低/高阈值、上报周期、GPS 定位周期。
- DC200 系列：融合模式、仅地磁模式、雷达优先模式和上报周期。
- SVC100：电压输出、接口功能模式、抖动持续时间、自动上电、时区和上报周期。
- 通用功能：实时任务、定时任务、设备校准、电源控制和时间戳同步；时间戳同步当前不对 UDS100 开放。

数值边界以 `DeviceActivity` 中的校验为准：常用上报周期为 1-1440 分钟，UDS100 满溢低阈值为 30-4500 mm，高阈值允许 0 或 30-4500 mm，GPS 周期允许 0 或 10-1440 分钟，SVC100 抖动持续时间为 1-255。

### 状态与诊断

- 连接后由 `StreamThread` 约每秒轮询一次设备状态。
- 展示电池、温湿度、距离、GPS、地磁三轴和雷达频谱等协议返回的数据。
- About 面板显示应用版本和系统错误日志。
- Debug 页面查看原始 BLE 通信数据，用于协议排查。
- 崩溃日志写入公共 Download 目录或应用私有目录，便于定位问题。

### 固件升级

- 通过 Android 文件选择器读取固件文件，Android 11 及以上使用 SAF URI 直接读取。
- OTA 按协议分包传输，每包 128 字节，末包补齐到 8 字节边界。
- 显示所选固件名、文件大小和升级进度。

## 构建与测试

技术栈：Kotlin 1.8.0、AGP 7.4.2、Gradle Wrapper 8.5、JDK 17、compile/target SDK 34、min SDK 26。应用要求 Android 8.0 及以上，并且手机必须支持 BLE。

Windows 环境优先使用：

```powershell
./build.ps1 assembleProdDebug
./build.ps1 assembleDevDebug
./build.ps1 testDevDebugUnitTest
./build.ps1 lint
```

构建变体为 `{dev, prod} x {debug, release}`，产物位于 `app/build/outputs/apk/<flavor>/<buildType>/`。`dev` 变体会追加 `.dev` 包名后缀。当前仓库的 `gradle.properties` 固定指向 Windows JDK 17 路径 `D:\Java\jdk-17`；在其他系统构建时需要先按本机 JDK 位置调整该配置。

现有测试包括 `BluetoothScanFilterTest` 单元测试、`ExampleUnitTest`、`ScanActivityTest` 和 `ExampleInstrumentedTest`。BLE 扫描、GATT 连接、协议收发和 OTA 需要真机及真实设备验证；模拟器只能验证界面、权限和基本启动流程。

Release 当前启用 R8 压缩和资源收缩，但 `app/build.gradle.kts` 未配置签名，因此 release 产物需要另行签名。仓库中的 `key.jks`、根目录 APK 和 `app/release` 元数据未被 Git 跟踪；其中 `app/release/output-metadata.json` 仍显示旧版本 2.1，不能作为当前版本依据。

## 目录结构

| 路径 | 内容 |
| --- | --- |
| `app/src/main/java/com/hkt/ble/bletools` | Kotlin 应用源码 |
| `app/src/main/res` | Android 布局、菜单、图标和文案资源 |
| `docs` | 参数协议、扫码逻辑、构建和发布说明 |
| `gradle` | Gradle Wrapper 配置 |
| `更新记录.txt` | 应用历史版本记录 |

`Communicate.kt` 是自定义协议、状态轮询和 OTA 组包的核心文件；`MainActivity.kt` 负责扫描、权限和连接；`DeviceActivity.kt` 负责设备配置和状态展示。修改协议、UI 或新增设备类型前，建议先阅读 [CLAUDE.md](CLAUDE.md) 和 [AGENTS.md](AGENTS.md)。

## 当前限制

- 应用为单设备连接模型，一次只维护一个 GATT 连接。
- 协议状态保存在全局可变对象中，后台线程、BLE 回调和 UI 线程会并发读写；修改时需遵循现有快照刷新方式。
- `ExcelUtils.kt` 使用 Apache POI 生成 `.xlsx`，但当前只写入示例数据，且没有界面调用，不能作为已交付的数据导出功能。
- 应用提供 `values-zh` 中文资源，但部分菜单和提示仍硬编码英文，双语覆盖尚未完成。
