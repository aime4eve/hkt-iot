# BLETools 参数配置与参数展示代码逻辑分析

## 一、项目概述

BLETools 是一个 Android 蓝牙低功耗（BLE）设备管理应用，主要用于连接和管理三类物联网设备：
- **UDS100**：垃圾桶满溢监测传感器
- **DC200**：地磁传感器
- **SVC100**：电磁阀控制器

---

## 二、参数配置（Parameter Configuration）模块

### 2.1 本地参数存储

#### SPUtils.kt - SharedPreferences 工具类

**文件位置**：`app/src/main/java/com/hkt/ble/bletools/SPUtils.kt`

**功能**：提供 SharedPreferences 的扩展函数，用于本地参数持久化存储。

```kotlin
const val NAME = "config"  // SharedPreferences 文件名

// 扩展函数封装
fun Boolean.putBoolean(key: String)
fun getBoolean(key: String, result: Boolean = false): Boolean
fun String?.putString(key: String)
fun getString(key: String, result: String? = null): String?
fun Int.putInt(key: String)
fun getInt(key: String, result: Int = 0): Int
fun Long.putLong(key: String)
fun getLong(key: String, result: Long = 0): Long
```

**使用示例**（MainActivity.kt）：
```kotlin
// RSSI 过滤阈值配置
val initialRssi = getInt(RSSI, 100)
sbRssi?.progress = initialRssi
seekBar.progress.putInt(RSSI)
```

### 2.2 BLE 协议参数配置

#### BLEUtils.kt - UUID 配置

**文件位置**：`app/src/main/java/com/hkt/ble/bletools/BLEUtils.kt`

**BLE UUID 常量**：
```kotlin
const val SERVICE_UUID = "0783B03E-8535-B5A0-7140-A304D2495CB7"
const val DESCRIPTOR_UUID = "00002902-0000-1000-8000-00805F9B34FB"
const val CHARACTERISTIC_WRITE_UUID = "0783B03E-8535-B5A0-7140-A304D2495CBA"
const val CHARACTERISTIC_INDICATE_UUID = "0783B03E-8535-B5A0-7140-A304D2495CB8"
const val RSSI = "rssi"  // RSSI 阈值存储键
```

### 2.3 设备参数配置数据结构

#### Communicate.kt - 参数数据结构

**文件位置**：`app/src/main/java/com/hkt/ble/bletools/Communicate.kt`

**1. DeviceTypeData（设备原始数据）**：
```kotlin
data class DeviceTypeData(
    var name: Int = 0,              // 设备类型
    var hardVersion: Int = 0,       // 硬件版本
    var softVersion: Int = 0,       // 软件版本
    var battery: Int = 0,           // 电池电量
    var power: Int = 0,             // 电源状态
    var temperature: Float = 0.0F, // 温度
    var humidity: Float = 0.0F,     // 湿度
    var angle: Float = 0.0F,        // 角度
    var distance: Int = 0,          // 距离
    var overflowState: Int = 0,     // 满溢状态
    var overflowLowThreshold: Int = 0,   // 低阈值
    var overflowHighThreshold: Int = 0, // 高阈值
    var latitude: Double = 0.0,     // 纬度
    var longitude: Double = 0.0,   // 经度
    var reportPeriod: Int = 0,      // 上报周期
    var gpsPeriod: Int = 0,         // GPS 周期
    // SVC100 特有
    var value1State: Int = 0,       // 端口1状态
    var pulse1Count: Int = 0,       // 脉冲计数
    var volOut: Int = 0,            // 电压输出
    var valveMode: Int = 0,         // 阀门模式
    var buffetingDuration: Int = 0, // 防抖时长
    var timeZone: Int = 0           // 时区
)
```

**2. DeviceTypeDataString（展示用字符串数据）**：
```kotlin
data class DeviceTypeDataString(
    var name: String = "",
    var version: String = "",       // 格式：V1.0
    var battery: String = "",       // 格式：85%
    var power: String = "",         // ON/OFF
    var temperature: String = "",   // 格式：25.5℃
    var humidity: String = "",      // 格式：60%
    // ... 其他字段
)
```

**3. DeviceEventData（设备事件/命令数据）**：
```kotlin
data class DeviceEventData(
    var event: Int = 0,                    // 事件类型
    var overflowLowThreshold: Int = 0,      // 低阈值配置
    var overflowHighThreshold: Int = 0,     // 高阈值配置
    var reportPeriod: Int = 0,             // 上报周期配置
    var gpsPeriod: Int = 0,                // GPS 周期配置
    var power: Int = 0,                    // 开关机配置
    var parkMode: Int = 0,                 // 停车模式配置
    // SVC100 定时任务
    var idTimed: Int = 0,
    var valveTimed: Int = 0,
    var stateTimed: Int = 0,
    var pulseTimed: Int = 0,
    var startTimeTimed: Int = 0,
    var endTimeTimed: Int = 0,
    var repeatTimed: Int = 0,
    // SVC100 实时任务
    var valveRealtime: Int = 0,
    var stateRealtime: Int = 0,
    var pulseRealtime: Int = 0,
    var timeRealtime: Int = 0,
    var Timestamp: Int = 0                 // 时间戳
)
```

### 2.4 设备事件枚举

```kotlin
enum class DeviceEventEnum {
    VALUE_NULL,
    // 开关机
    POWER_ON_EVENT, POWER_ON_START_EVENT, POWER_ON_FINISH_EVENT,
    POWER_OFF_EVENT, POWER_OFF_START_EVENT, POWER_OFF_FINISH_EVENT,
    // 校准
    CALIBRATION_EVENT, CALIBRATION_START_EVENT, 
    CALIBRATION_EXECUTE_EVENT, CALIBRATION_FINISH_EVENT,
    // 参数配置
    CONFIG_PARAMETER_EVENT, CONFIG_PARAMETER_START_EVENT, CONFIG_PARAMETER_FINISH_EVENT,
    // 时间同步
    SYNC_TIMESTAMP_EVENT, SYNC_TIMESTAMP_START_EVENT, SYNC_TIMESTAMP_FINISH_EVENT,
    // OTA 升级
    SELECT_FILE, START_OTA, ENTER_OTA, UPDATING_OTA,
    // 同步
    SYNC_EVENT,
    // SVC100 定时任务
    CONFIG_TIMED_TASK, CONFIG_TIMED_TASK_START_EVENT, CONFIG_TIMED_TASK_FINISH_EVENT,
    DELETE_TIMED_TASK, DELETE_TIMED_TASK_START_EVENT, DELETE_TIMED_TASK_FINISH_EVENT,
    // SVC100 实时任务
    CONFIG_REALTIME_TASK, CONFIG_REALTIME_TASK_START_EVENT, CONFIG_REALTIME_TASK_FINISH_EVENT
}
```

### 2.5 设备配置 UI

#### DeviceActivity.kt - 参数配置界面

**文件位置**：`app/src/main/java/com/hkt/ble/bletools/DeviceActivity.kt`

**配置对话框类型**：

| 设备类型 | 配置项 | 对话框布局 |
|---------|--------|-----------|
| UDS100 | 电源开关、校准、阈值/周期配置、时间同步、OTA | `dialog_config_uds100.xml` |
| DC200 | 电源开关、校准、周期/模式配置、时间同步、OTA | `dialog_config_dc200.xml` |
| SVC100 | 电源、阀门参数、实时任务、定时任务、时间同步、OTA | `dialog_config_svc100.xml` |

**配置参数校验规则**：
```kotlin
// UDS100 参数校验
overflowHighThreshold: 30-4500 mm
overflowLowThreshold: 30-4500 mm  
reportPeriod: 1-1440 min
gpsPeriod: 10-1440 min

// DC200 参数校验
reportPeriod: 1-1440 min

// SVC100 参数校验
reportPeriod: 1-1440 min
buffetingDuration: 1-255
pulse: 0-65535
time: 0-65535
```

---

## 三、参数展示（Parameter Display）模块

### 3.1 蓝牙扫描结果展示

#### MainActivity.kt - 设备列表

**文件位置**：`app/src/main/java/com/hkt/ble/bletools/MainActivity.kt`

**展示逻辑**：
```kotlin
// 设备列表数据模型
data class BleDevice(var device: BluetoothDevice, var rssi: Int, var name: String?)

// RecyclerView 适配器
class RecyclerViewListAdapter : RecyclerView.Adapter<ViewHolder>()

// 设备信息展示（item_bluetooth.xml）
- tv_device: 设备名称
- tv_mac_address: MAC 地址
- tv_rssi: 信号强度（如：-65 dBm）
```

**扫描过滤配置**：
```kotlin
// 过滤条件
isScanNullNameDevice: Boolean  // 是否过滤名称为 null 的设备
rssi: Int                      // RSSI 阈值（-100 ~ 0）

// 过滤方法
MainActivity.shouldFilterDevice(isScanNullNameDevice, deviceName)
```

### 3.2 设备状态参数展示

#### DeviceActivity.kt - 可展开列表展示

**文件位置**：`app/src/main/java/com/hkt/ble/bletools/DeviceActivity.kt`

**展示组件**：`ExpandableListView` + `ExpandableListAdapter`

**数据模型**：
```kotlin
data class Group(val id: Int, val title: String, val groupType: Int)
data class Child(val id: Int, val groupId: Int, val title: String, val word: String)
```

**UDS100 状态展示项**：
```
Status:
├── Name: 设备名称
├── Version: V1.0
├── Power: ON/OFF
├── Battery: 85%
├── Temperature: 25.5℃
├── Humidity: 60%
├── Alarm Status: Normal
├── Angle: 45.0°
├── Slant: Normal
├── Distance: 500mm
├── Overflow State: Normal
├── Low Threshold: 100mm
├── High Threshold: 400mm
├── Latitude: 39.9042
├── Longitude: 116.4074
├── Report Period: 60min
└── Gps Period: 30min
```

**DC200 状态展示项**：
```
Status:
├── Name
├── Version
├── Power
├── Battery
├── Parking Status: Hold/Idle
├── Tamper Status: Alarm/Normal
└── Report Period
```

**SVC100 状态展示项**：
```
Status:
├── Name
├── Version
├── Power
├── Battery
├── Port1 value state: OPEN/OFF
├── Port1 insert state: Connect/Disconnect
├── Port1 pulse count: 12345
├── Port2 value state: OPEN/OFF
├── Port2 insert state: Connect/Disconnect
├── Port2 pulse count: 67890
├── Voltage out level: 12V/9V/5V
├── Interface function: 1&2 pulse mode 等
├── Buffeting duration: 50
├── Timezone: UTC+8
└── Report Period
```

### 3.3 调试日志展示

#### DebugActivity.kt

**文件位置**：`app/src/main/java/com/hkt/ble/bletools/DebugActivity.kt`

**功能**：
- 显示 BLE 通信原始数据
- 自定义指令发送
- 实时日志滚动

**UI 组件**：
```kotlin
// activity_debug.xml
- et_command: 命令输入框
- bt_send_cmd: 发送按钮
- tv_debug: 日志显示区域（ScrollView 包裹）
- scroll: 滚动容器
```

---

## 四、数据流与通信协议

### 4.1 BLE 通信流程

```
App (StreamThread)  ──sendCommand()──>  BLE Device
    │                                            │
    │<─── onCharacteristicChanged() ─────────────┤
    │                                            │
    └── streamRev() 解析数据 ──> mDeviceData ──> mDeviceDataString
```

### 4.2 协议格式

**下行指令格式**：
```
hkt(3) packnum(1) len(2) cmd(1) data(n) crc(2)
```

**命令码**：
| 命令码 | 功能 |
|--------|------|
| 0xFF | 查询设备状态 |
| 0xFE | 开/关机配置 |
| 0xFD | 校准 |
| 0x02 | 参数配置 |
| 0x03 | 实时任务配置 |
| 0x04 | 定时任务配置 |
| 0x05 | 删除定时任务 |
| 0x06 | 时间同步 |

### 4.3 数据解析

**streamRev() 函数解析设备上报数据**：

```kotlin
// 设备状态查询响应 (0xFF)
cmd = 0x01: 硬软件版本
cmd = 0x03: 电池电量
cmd = 0x09: 温度
cmd = 0x0A: 湿度
cmd = 0x0E: 角度
cmd = 0x10: 纬度
cmd = 0x11: 经度
cmd = 0x28: 温湿度告警
cmd = 0x3A: 停车状态
cmd = 0x3C: SVC100 端口状态
cmd = 0x40: 电压输出等级
cmd = 0x41: 阀门模式
cmd = 0x42: 防抖时长
cmd = 0x45: GPS 周期
cmd = 0x46: 超声波距离
cmd = 0x47: 满溢状态
cmd = 0x48: 满溢阈值
cmd = 0x84: 告警状态
cmd = 0x86: 上报周期
cmd = 0x8A: 时区
cmd = 0x8D: 电源状态
```

---

## 五、关键类图

```
┌─────────────────────────────────────────────────────────────┐
│                        全局数据对象                          │
├─────────────────────────────────────────────────────────────┤
│  mDeviceData: DeviceTypeData         设备原始数据            │
│  mDeviceDataString: DeviceTypeDataString  展示用字符串数据   │
│  mDeviceEvent: DeviceEventData        事件/命令数据         │
└─────────────────────────────────────────────────────────────┘
                              │
         ┌────────────────────┼────────────────────┐
         ▼                    ▼                    ▼
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│   SPUtils.kt    │  │   BLEUtils.kt   │  │ Communicate.kt  │
│  本地参数存储   │  │   BLE UUID配置  │  │  数据结构定义   │
│  - RSSI阈值    │  │  - Service     │  │  - DeviceType  │
│  - 过滤设置    │  │  - Character   │  │  - DeviceEvent │
└─────────────────┘  └─────────────────┘  └─────────────────┘
         │                    │                    │
         └────────────────────┼────────────────────┘
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                     UI 层 (Activity)                        │
├─────────────────────────────────────────────────────────────┤
│  MainActivity          DeviceActivity         DebugActivity │
│  - 扫描列表展示        - 参数配置对话框      - 调试日志     │
│  - 扫描过滤设置        - 状态参数展示        - 指令发送     │
└─────────────────────────────────────────────────────────────┘
```

---

## 六、文件结构汇总

| 文件路径 | 功能描述 |
|---------|---------|
| `SPUtils.kt` | SharedPreferences 工具类 |
| `BLEUtils.kt` | BLE 连接回调、UUID 配置、指令发送辅助 |
| `Communicate.kt` | 数据结构定义、协议编解码 |
| `MainActivity.kt` | 蓝牙扫描、设备列表展示 |
| `DeviceActivity.kt` | 设备状态展示、参数配置 |
| `DebugActivity.kt` | 调试界面、日志展示 |

---

## 七、总结

本项目的参数配置与展示采用了清晰的分层架构：

1. **数据层**：`Communicate.kt` 定义了完整的数据结构，将原始二进制数据转换为业务可用的数据对象
2. **配置层**：`SPUtils.kt` 负责本地配置持久化，`BLEUtils.kt` 负责 BLE 协议配置
3. **展示层**：通过 `ExpandableListView` 展示设备状态，支持多种设备类型的差异化展示
4. **通信层**：基于 BLE 透传协议，实现命令下发与数据解析

这种架构设计使得应用能够灵活支持多种 BLE 设备，同时保持代码结构清晰易维护。
