# HKT IoT 设备固件

本目录收录 LoRaWAN 设备固件工程。大多数固件使用 Keil MDK5 构建，入口为各产品 `USER` 目录下的 `.uvprojx` 文件；人员计数发射端使用 IAR EWARM，入口为 `.eww`/`.ewp` 文件。`USER/Firmware` 或 `USER/Debug/Exe` 保存历史固件产物，`Docs` 或 `Doc` 保存硬件与协议资料。

| 设备 | 产品型号 | 代码目录 | MCU | 工程状态 |
| --- | --- | --- | --- | --- |
| 地磁车位传感器 | MPS100、EPS100 | [LoRaWAN_ParkingSensor](LoRaWAN_ParkingSensor/) | STM32L431RCTx | 应用与 Bootloader 工程齐全 |
| 智能空开 | SCB100 | [LoRaWAN_Air_Switch](LoRaWAN_Air_Switch/) | GD32F103C8 | 应用工程齐全 |
| 车位锁 | CL100 | [LoRaWAN_Car_Lock](LoRaWAN_Car_Lock/) | STM32L051C8Tx | 应用工程齐全 |
| 三合一空气质量传感器 | IAQ300 | [LoRaWAN_IAQ300](LoRaWAN_IAQ300/) | FM33A06XEV | 应用工程齐全 |
| 多参数空气质量传感器 | IAQ300、IAQ600、IAQ900；解码协议为 AQS1000 | [LoRaWAN_IAQ1000](LoRaWAN_IAQ1000/) | FM33A06XEV | 应用与 Bootloader 工程齐全 |
| 温湿度传感器 | GT30 | [LoRaWAN_THTB_Sensor](LoRaWAN_THTB_Sensor/) | STM32L051C8Tx | 应用工程齐全 |
| 垃圾桶满溢监测传感器 | UDS100 | [LoRaWAN_Ultrasonic_Distance_Sensor](LoRaWAN_Ultrasonic_Distance_Sensor/) | STM32L431RCTx | 应用与 Bootloader 工程齐全 |
| 电磁阀控制器 | SVC100 | [LoRaWAN_Solenoid_Valve_Controller](LoRaWAN_Solenoid_Valve_Controller/) | STM32L431RCTx | 应用与 Bootloader 工程齐全 |
| 瘤胃胶囊 | SRB100 | [LoRaWAN_Smart_Rumen_Bolus](LoRaWAN_Smart_Rumen_Bolus/) | STM32L051C8Tx | 应用工程齐全 |
| 牛羊定位器 | AT100、GAT100 | [LoRaWAN_Animals_Track](LoRaWAN_Animals_Track/) | STM32L051C8Tx | 源码和历史产物已入库，但 `.uvprojx` 工程文件缺失 |
| 红外人流量计数器 | PC100（接收端）；发射端文件未标明独立型号 | [接收端](LoRaWAN_People_Counter/LoRaWAN_People_Counter_Recv/)、[发射端](LoRaWAN_People_Counter/LoRaWAN_People_Counter_Tx/) | 接收端 STM32L051C8Tx；发射端 STM8L101F2P | 接收端 Keil 应用工程齐全；发射端 IAR EWARM 应用工程与调试产物齐全 |
| 智能门锁 | SDL100、SDL200 | [LoRaWAN_SmartDoorLock](LoRaWAN_SmartDoorLock/) | FM33A06XEV | 应用与 Bootloader 工程齐全 |
| 烟感探测器 | SD300 | [LoRaWAN_SmokeAlarm_KWX](LoRaWAN_SmokeAlarm_KWX/) | HT32F52241_48LQFP | 应用工程与历史固件齐全 |

各工程通常按以下结构组织：

- `USER`: 应用代码、Keil/IAR 工程和历史固件产物。
- `Compents`: MCU 库、LoRaWAN 模块、传感器驱动和部分平台的 payload 解码脚本。
- `Docs` 或 `Doc`: 原理图、通信协议、芯片手册等产品资料。
