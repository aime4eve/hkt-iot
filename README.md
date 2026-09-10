# HKT IoT

本仓库收录华宽通 IoT 的 Android 蓝牙配置工具和 LoRaWAN 设备固件工程。根目录只做项目索引，两个子项目各自维护构建入口、详细说明和历史记录。

| 项目 | 内容 | 技术栈 / 构建方式 | 详细说明 |
| --- | --- | --- | --- |
| [HKT-BLETools](HKT-BLETools/) | Android 蓝牙配置工具，当前版本 3.16，支持设备扫描、连接、参数配置、状态查看和 OTA 升级 | Kotlin 1.8.0、AGP 7.4.2、Gradle 8.5、JDK 17 | [HKT-BLETools/Readme.md](HKT-BLETools/Readme.md) |
| [HKT-Firmwares](HKT-Firmwares/) | 自研固件 13 个产品目录（12 个有应用工程）、外协产品资料，以及历史固件产物和硬件/协议资料 | 以 Keil MDK5 为主，人员计数发射端为 IAR EWARM | [HKT-Firmwares/Readme.md](HKT-Firmwares/Readme.md) |

## 设备覆盖

HKT-BLETools 当前支持 UDS100、DC200 系列、SVC100：

| 设备 | BLETools 支持 | 固件目录 |
| --- | --- | --- |
| UDS100 垃圾桶满溢监测传感器 | 支持 | [LoRaWAN_Ultrasonic_Distance_Sensor](HKT-Firmwares/in-house/LoRaWAN_Ultrasonic_Distance_Sensor/) |
| EPS100 / MPS100 地磁车位传感器 | 归入 DC200 系列界面 | [LoRaWAN_ParkingSensor](HKT-Firmwares/in-house/LoRaWAN_ParkingSensor/) |
| SVC100 电磁阀控制器 | 支持 | [LoRaWAN_Solenoid_Valve_Controller](HKT-Firmwares/in-house/LoRaWAN_Solenoid_Valve_Controller/) |

BLETools 的地磁名称匹配包含 `DC200`、`EPS100`、`MPS100`；当前固件索引列出的是 EPS100 和 MPS100，没有单独的 DC200 工程目录。

固件仓库按来源分为 [in-house](HKT-Firmwares/in-house/)（自研）和 [out-sourced](HKT-Firmwares/out-sourced/)（外协），自研产品包括智能空开、车位锁、空气质量传感器、温湿度传感器、瘤胃胶囊、牛羊定位器、人流量计数器和智能门锁等，完整型号、MCU 和工程状态见固件目录 README。

## 快速开始

### HKT-BLETools

Android 工程要求 JDK 17 和 Android SDK，`minSdk` 为 26。Windows 环境优先使用项目内脚本：

```powershell
cd HKT-BLETools
./build.ps1 assembleProdDebug
./build.ps1 testDevDebugUnitTest
```

BLE 扫描、GATT 连接、协议收发和 OTA 需要真机及真实设备验证；模拟器只适合验证界面、权限和基本启动流程。

### HKT-Firmwares

固件工程以 Keil MDK5 为主，人员计数发射端使用 IAR EWARM。进入具体产品目录，打开 `USER` 目录下对应的 `.uvprojx` 或 `.eww` 文件构建；包含 OTA 的产品通常还提供 Bootloader 工程。部分产品目录存在历史限制，例如牛羊定位器源码已入库但缺少 `.uvprojx` 文件，构建前先查看对应 README 和实际工程入口。

## 项目边界

- 根目录没有统一构建脚本，也不负责打包 Android APK 或固件。
- Android 版本、设备支持、协议和测试范围以 [HKT-BLETools/Readme.md](HKT-BLETools/Readme.md) 为准。
- 固件产品型号、MCU、工程入口和历史产物以 [HKT-Firmwares/Readme.md](HKT-Firmwares/Readme.md) 为准。
