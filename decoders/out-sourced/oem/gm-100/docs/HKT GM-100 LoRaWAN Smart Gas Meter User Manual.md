# HKT GM-100 LoRaWAN Smart Gas Meter User Manual

## 5 Data Communication Protocol

### 5.1 Communication Protocol Data Structure

All data are expressed in HEX format


| Sync Head (3 bytes) | Special Type (1 byte) | Data packet serial number (1 byte) | Data type (1 byte) | Data (n bytes) | N（Data Type+Data）(1+n+1+n+...) |
| ------------------- | --------------------- | ---------------------------------- | ------------------ | -------------- | ------------------------------ |


---

### 5.2 Communication Protocol Analysis


| Protocol Field Name       | Description                                                                                                                                                                                                                                                                         |
| ------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Sync head                 | The synchronization header is a fixed 3 bytes of data (0x68 0x6B 0x74) taken from "HKT"                                                                                                                                                                                             |
| Special type              | The special type is a fixed 1 bytes length of data with BIT bits representing the specific function; BIT0: Used to inform the device or server if an answer or acknowledgement packet is required (0: no answer required 1: answer required); BIT1~BIT7: function to be determined. |
| Data packet serial number | The data packet serial number is a fixed length of 1 bytes, which is used to identify the data serial number.                                                                                                                                                                       |
| Data type                 | The data type is fixed 1 bytes length data, which is mainly used to identify different functional types of data of the device.                                                                                                                                                      |
| Data                      | The data is n bytes variable length data, and the length of the data content is confirmed according to different data types.                                                                                                                                                        |


---

### 5.3 Data Type Table


| Data type | Function                             | Note                                                                                                                                                                                                                                                                                                                           |
| --------- | ------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 0x01      | Device software and hardware version | The data length is fixed at 2 bytes, and the uplink is automatically synchronized when the power is turned on again. Only uplink is supported. The first 1 byte represents the hardware version, the last 1 byte represents the software version Example: Sync hardware version 1, software version 5: 68 6B 74 00 01 01 01 05 |
| 0x03      | Device Battery Information           | Upload based on battery percentage, only supports uplink Example: Synchronized power information: 68 6B 74 00 03 03 64 (power 100%)                                                                                                                                                                                            |
| 0x33      | Gas Unit Price                       | The data length is fixed at 4 bytes and only supports downlink. Unit: 0.01/m³ Example: Modify the gas unit price to 2.56/m³: 68 6B 74 00 01 33 00 00 01 00                                                                                                                                                                     |
| 0x34      | Gas recharge payment                 | The data length is fixed at 4 bytes and only supports downlink. Unit: 0.01/m³ Example: Recharging Balance Account: 128.20/m³: 68 6B 74 00 01 34 00 00 32 14                                                                                                                                                                    |
| 0x35      | Total usage                          | The data length is fixed at 4 bytes and supports uplink/downlink. Unit: 0.01m³ Example: Total gas usage 128.20m³: 68 6B 74 00 01 35 00 00 32 14                                                                                                                                                                                |
| 0x36      | Gas remaining amount                 | The data length is fixed at 4 bytes and supports uplink/downlink. Unit: 0.01m³ Example: Modify remaining gas to 128.20m³: 68 6B 74 00 01 36 00 00 32 14                                                                                                                                                                        |
| 0x37      | Gas balance                          | The data length is fixed at 4 bytes and supports uplink/downlink. Unit: 0.01m³ Example: Modify gas balance to 128.20/m³: 68 6B 74 00 01 37 00 00 32 14                                                                                                                                                                         |
| 0x38      | Valve status                         | The data length is fixed at 1 byte and supports uplink/downlink. 00: Valve closed; 01: Valve open Example: Report valve open: 68 6B 74 00 01 38 01                                                                                                                                                                             |
| 0x80      | Synchronize system time              | Uplink: request; Downlink format: year, month, day, hour, minute, second Example: Request time: 68 6B 74 01 01 80 02 1C 0C 00 Server downlink 2022-03-28 12:00: 68 6B 74 00 08 80 16 02 1C 0C 00                                                                                                                               |
| 0x83      | Fault Status                         | Fixed 1 byte, uplink only 0: fault cleared; 1: device failure / counting interference alarm Example: Normal status: 68 6B 74 00 01 83 00                                                                                                                                                                                       |
| 0x85      | Resume factory settings              | Downlink only 1: restore factory settings Example: 68 6B 74 00 0A 85 01                                                                                                                                                                                                                                                        |
| 0x86      | Data Synchronization Period          | Fixed 2 bytes, uplink/downlink; Unit: minute; Range:10–1440 Example: Set 1440 minutes: 68 6B 74 00 01 86 05 A0                                                                                                                                                                                                                 |
| 0x89      | Battery status                       | Fixed 1 byte, uplink only 0: under voltage alarm; 1: normal Example: Normal power: 68 6B 74 00 01 89 01                                                                                                                                                                                                                        |


---

### 5.4 Example

#### Device reporting cycle data

Full frame:

`68 6B 74 00 04 01 0C 01 03 33 33 00 00 00 00 35 00 00 00 00 36 00 00 03 E8 37 00 00 03 E8 38 00 83 00 89 00 86 00 1E`


| Field               | Value       | Description          |
| ------------------- | ----------- | -------------------- |
| Sync head           | 68 6B 74    | Fixed sync header    |
| Special type        | 00          | No need confirmation |
| Serial number       | 04          | Packet No.04         |
| 01 (Version)        | 0C 01       | HW:0C, SW:01         |
| 03 (Battery)        | 33          | 51%                  |
| 33 (Unit price)     | 00 00 00 00 | 0/m³                 |
| 35 (Total usage)    | 00 00 00 00 | 0/m³                 |
| 36 (Remaining)      | 00 00 03 E8 | 10/m³                |
| 37 (Balance)        | 00 00 03 E8 | 10 ¥                 |
| 38 (Valve)          | 00          | Closed               |
| 83 (Fault)          | 00          | Normal               |
| 89 (Battery status) | 00          | Normal               |
| 86 (Sync period)    | 00 1E       | 30 minutes           |


> （注：文档部分内容可能由 AI 生成）

