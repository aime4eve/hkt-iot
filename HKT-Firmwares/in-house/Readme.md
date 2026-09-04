# in-house 自研固件

本目录收录华宽通自研的 LoRaWAN 设备固件工程，共 13 个产品目录，其中 12 个有可构建应用工程。完整设备索引（产品型号、MCU、工程状态与构建方式）见上级 [Readme.md](../Readme.md)；外协产品资料在 [../out-sourced](../out-sourced/)。

各工程通常按以下结构组织：

- `USER`: 应用代码、Keil/IAR 工程和历史固件产物。
- `Compents`: MCU 库、LoRaWAN 模块、传感器驱动和部分平台的 payload 解码脚本。
- `Docs` 或 `Doc`: 原理图、通信协议、芯片手册等产品资料。

构建入口为各产品 `USER` 目录下的 `.uvprojx`（Keil MDK5）或 `.eww`/`.ewp`（IAR EWARM，人员计数发射端）。部分产品目录内有各自的 README.md，记录该产品的构建、烧录与注意事项；牛羊定位器源码和历史产物已入库，但缺少 `.uvprojx` 工程文件。
