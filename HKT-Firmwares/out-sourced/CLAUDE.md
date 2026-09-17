# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

物联网设备 LoRaWAN 协议负载解码工具集，用于解析和调试 LoRaWAN 设备（水表、阀门、采集器、压力计等）上报的二进制数据。

## 目录结构

```
负载解码/
├── 水表/
│   ├── 汇中/              # 统一协议（帧头 0x66）- 主要开发目录
│   │   ├── uni-decode-en.js    # 英文版解码器（默认）
│   │   ├── uni-decode-cn.js    # 中文版解码器
│   │   ├── test-node.js        # 本地测试脚本
│   │   ├── encode-test.txt     # 测试输入
│   │   └── LoRaWAN-tools.html  # 网页调试工具
│   └── 中配/              # HKT 协议（帧头 0x68）
│       └── hkt_water_meter_decoder.js
└── backup/               # 历史版本存档
```

**主要工作目录**: `水表/汇中/`

## 开发命令

```bash
# 进入主要工作目录
cd 水表/汇中

# 本地测试解码器
node test-node.js

# 输出文件：test-result.txt
```

## 协议规范（统一协议 - 汇中）

### 字节序
小端序（Little Endian）：低位在前，高位在后

### 帧结构
```
帧起始符(0x66) | 长度L | 协议版本V | 地址域A(7字节) | 帧序号SEQ | 设备类型 | 控制域C | 辅助控制域CI | 数据区 | 帧校验和CS | 帧结束符(0x16)
```

### AFN 功能码（控制域C低4位）
- `0x01`: 实时数据上报
- `0x03`: 告警数据上报（Fn=6）
- `0x04`: 参数设置
- `0x05`: 参数读取
- `0x06`: 控制命令

### 设备类型
- `0x01`: 阀门
- `0x02`: 采集器
- `0x03`: 压力
- `0x04`: 水表

### 数据类别（Category ID）
- `1`: 终端状态（供电电压、DEVEUI、RSSI等）
- `2`: 实时数据（瞬时流量、累积流量、温度、压力等）
- `3`: 冻结数据
- `4`: 日结数据
- `5`: 曲线数据
- `6`: 告警信息
- `30`: 日志数据

### 数据类型
| 类型 | 说明 |
|------|------|
| `FLOAT` | 4字节 IEEE 754 浮点数 |
| `UINT8/16/32` | 无符号整数 |
| `INT16` | 有符号整数 |
| `BCD_TIME` | BCD 编码时间（6字节：YYMMDDHHmmss，4字节：YYMMDDHH） |
| `BS8` | 变长格式：格式字节(高4位=数据类型, 低4位=小数位) + 数据 |
| `UNIX_TIME` | 4字节 Unix 时间戳 |

### ID头字节结构
```
位7-5: LID（数据长度标识，0=扩展长度）
位4-0: ID（数据项标识，0=扩展ID）
```
- LID 1-7 对应数据长度 1,2,3,4,5,6,16 字节
- LID=0 时需读取 ELID 字节计算扩展长度

## 解码器接口

```javascript
// ChirpStack v4 标准接口
function decodeUplink(input) {
  // input.bytes: Uint8Array
  // returns: { data: { ... } }
}
```

### 输出格式
扁平 JSON 对象，包含：
- 帧头信息：`startFlag`, `length`, `version`, `address`, `seq`, `deviceType`, `control`, `ci`
- AFN 信息：`afn`, `afnText`, `dirText`, `prmText`
- 数据统计：`alarmCount`, `dataBlocks`
- 数据项：按解析结果动态生成键名（如 `Supply Voltage`, `Instant Flow Rate` 等）
- 错误时返回 `{ data: { error: "错误信息" } }`

## 使用方式

1. **网页工具**：浏览器直接打开 `水表/汇中/LoRaWAN-tools.html`
2. **ChirpStack 集成**：将 `uni-decode-cn.js` 或 `uni-decode-en.js` 代码复制到 Device Profile → Codec 配置中

## 切换解码器版本

`test-node.js` 默认引用英文版，如需切换：

```js
const decoder = require("./uni-decode-en.js");  // 英文版（默认）
const decoder = require("./uni-decode-cn.js");  // 中文版
```
