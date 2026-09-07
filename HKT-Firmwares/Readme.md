# HKT-Firmwares

华宽通 LoRaWAN 设备固件与配套资料。目录按来源分为两部分：

- [in-house](in-house/): 自研固件工程，共 13 个产品目录，其中 12 个有可构建应用工程。
- [out-sourced](out-sourced/): 外协产品资料，当前为 PIR100 人存感应器的用户手册与上行解码脚本。

## 自研设备索引

| 设备 | 产品型号 | 代码目录 | MCU | 工程状态 |
| --- | --- | --- | --- | --- |
| 地磁车位传感器 | MPS100、EPS100 | [LoRaWAN_ParkingSensor](in-house/LoRaWAN_ParkingSensor/) | STM32L431RCTx | 应用与 Bootloader 工程齐全 |
| 智能空开 | SCB100 | [LoRaWAN_Air_Switch](in-house/LoRaWAN_Air_Switch/) | GD32F103C8 | 应用工程齐全 |
| 车位锁 | CL100 | [LoRaWAN_Car_Lock](in-house/LoRaWAN_Car_Lock/) | STM32L051C8Tx | 应用工程齐全 |
| 三合一空气质量传感器 | IAQ300 | [LoRaWAN_IAQ300](in-house/LoRaWAN_IAQ300/) | FM33A06XEV | 应用工程齐全 |
| 多参数空气质量传感器 | IAQ300、IAQ600、IAQ900；解码协议为 AQS1000 | [LoRaWAN_IAQ1000](in-house/LoRaWAN_IAQ1000/) | FM33A06XEV | 应用与 Bootloader 工程齐全 |
| 温湿度传感器 | GT30 | [LoRaWAN_THTB_Sensor](in-house/LoRaWAN_THTB_Sensor/) | STM32L051C8Tx | 应用工程齐全 |
| 垃圾桶满溢监测传感器 | UDS100 | [LoRaWAN_Ultrasonic_Distance_Sensor](in-house/LoRaWAN_Ultrasonic_Distance_Sensor/) | STM32L431RCTx | 应用与 Bootloader 工程齐全 |
| 电磁阀控制器 | SVC100 | [LoRaWAN_Solenoid_Valve_Controller](in-house/LoRaWAN_Solenoid_Valve_Controller/) | STM32L431RCTx | 应用与 Bootloader 工程齐全 |
| 瘤胃胶囊 | SRB100 | [LoRaWAN_Smart_Rumen_Bolus](in-house/LoRaWAN_Smart_Rumen_Bolus/) | STM32L051C8Tx | 应用工程齐全 |
| 牛羊定位器 | AT100、GAT100 | [LoRaWAN_Animals_Track](in-house/LoRaWAN_Animals_Track/) | STM32L051C8Tx | 源码和历史产物已入库，但 `.uvprojx` 工程文件缺失 |
| 红外人流量计数器 | PC100（接收端）；发射端文件未标明独立型号 | [接收端](in-house/LoRaWAN_People_Counter/LoRaWAN_People_Counter_Recv/)、[发射端](in-house/LoRaWAN_People_Counter/LoRaWAN_People_Counter_Tx/) | 接收端 STM32L051C8Tx；发射端 STM8L101F2P | 接收端 Keil 应用工程齐全；发射端 IAR EWARM 应用工程与调试产物齐全 |
| 智能门锁 | SDL100、SDL200 | [LoRaWAN_SmartDoorLock](in-house/LoRaWAN_SmartDoorLock/) | FM33A06XEV | 应用与 Bootloader 工程齐全 |
| 烟感探测器 | SD300 | [LoRaWAN_SmokeAlarm_KWX](in-house/LoRaWAN_SmokeAlarm_KWX/) | HT32F52241_48LQFP | 应用工程与历史固件齐全 |

## 构建方式

- 大多数固件使用 Keil MDK5 构建，入口为各产品 `USER` 目录下的 `.uvprojx` 文件；人员计数发射端使用 IAR EWARM，入口为 `.eww`/`.ewp` 文件。
- 车位传感器、多参数空气质量、垃圾桶满溢监测、电磁阀控制器和智能门锁同时提供 Bootloader 工程，支持 OTA 升级。
- `USER/Firmware` 或 `USER/Debug/Exe` 保存历史固件产物，`Docs` 或 `Doc` 保存原理图、通信协议、芯片手册等资料。
- 自研工程的目录组织约定见 [in-house/Readme.md](in-house/Readme.md)；部分产品目录内有各自的 README.md，记录构建与烧录说明。

## 外协产品

| 设备 | 产品型号 | 目录 | 内容 |
| --- | --- | --- | --- |
| 人存感应器 | PIR100 | [out-sourced/PIR100-人存感应器](out-sourced/PIR100-人存感应器/) | 用户手册 PDF，ChirpStack / TTN 上行解码脚本 |
