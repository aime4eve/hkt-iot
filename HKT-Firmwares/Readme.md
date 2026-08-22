# HKT IoT 设备固件

本目录收录 LoRaWAN 设备固件工程。除特别说明外，固件使用 Keil MDK5 构建，入口为各产品 `USER` 目录下的 `.uvprojx` 文件；`USER/Firmware` 保存历史固件产物，`Docs` 或 `Doc` 保存硬件与协议资料。

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
| 红外人流量计数器 | PC100 | [LoRaWAN_People_Counter/LoRaWAN_People_Counter_Recv](LoRaWAN_People_Counter/LoRaWAN_People_Counter_Recv/) | STM32L051C8Tx | 接收端应用工程齐全；`LoRaWAN_People_Counter_Tx` 当前为空目录 |
| 智能门锁 | SDL100、SDL200 | [LoRaWAN_SmartDoorLock](LoRaWAN_SmartDoorLock/) | FM33A06XEV | 应用与 Bootloader 工程齐全 |

烟感固件源码当前未入库。本地存在的 `LoRaWAN_SmokeAlarm_KWX` 为空目录，Git 不跟踪空目录，因此未列入上方产品索引。

各工程通常按以下结构组织：

- `USER`: 应用代码、Keil 工程和历史固件产物。
- `Compents`: MCU 库、LoRaWAN 模块、传感器驱动和部分平台的 payload 解码脚本。
- `Docs` 或 `Doc`: 原理图、通信协议、芯片手册等产品资料。
