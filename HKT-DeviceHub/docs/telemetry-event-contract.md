# TelemetryFrame 事件契约

DeviceHub 上行遥测的唯一输出契约。任何下游业务项目消费遥测都以本文档为准。

## 投递约定

| 项 | 值 |
|----|----|
| Topic | `DEVICEHUB_TELEMETRY_FRAME` |
| Tag | 设备所属项目：`LIVESTOCK` / `PARKING`（消费端按 tag 过滤） |
| Message Key | `frameId`（RocketMQ `PROPERTY_KEYS`，可用于 broker 侧按 key 查询） |
| Payload | `TelemetryFrame` JSON（UTF-8，`content-type: application/json`） |
| Consumer Group 约定 | `{PROJECT}_DEVICEHUB_TELEMETRY_{业务名}`，如 `LIVESTOCK_DEVICEHUB_TELEMETRY_CORE`。同一 group 内负载均衡；不同 group 各自收到全量 |
| Spring Cloud Stream binding | `telemetryFrame-out-0` |

## TelemetryFrame 字段表

| 字段 | 类型 | 可空 | 说明 |
|------|------|------|------|
| `version` | int | 否 | 契约版本，当前恒为 `1`；破坏性变更时升版 |
| `frameId` | string | 否 | 稳定去重键，见下节生成规则 |
| `devEui` | string | 否 | LoRaWAN DevEUI，小写 16 位 hex |
| `tbDeviceId` | string | 否 | ThingsBoard 设备 UUID |
| `ts` | long | 否 | 帧时间戳，epoch 毫秒（TB timeseries ts） |
| `properties` | object | 否（可为 `{}`） | result 权威帧的解码属性，原样透传 `decodeData.properties`，DeviceHub 不解释业务字段 |
| `dataHex` | string | 是 | 原始上行 hex；仅当 2s 窗口内无权威 result 帧时存在（fallback 帧） |
| `rssi` | int | 是 | 传输层元数据，TB 有值才带 |
| `snr` | int | 是 | 同上 |
| `gatewayId` | string | 是 | TB 上报的网关标识（`downLinkGateway` 键） |

序列化策略：`NON_NULL`——可空字段无值时不出现在 JSON 中。

## frameId 生成规则

```
frameId = UUID.nameUUIDFromBytes(UTF8(tbDeviceId + ":" + ts))   // UUID v3，SHA-1 name-based
```

特性：

- **稳定**：同一 `(tbDeviceId, ts)` 在任何通道（WS 推送 / REST 回补）、任何重放、任何实例上生成同一个 frameId。
- **无业务含义**：只是去重键，不要解析其内容。
- 同一物理帧的 `result` 与 `dataHex` 两个键由 DeviceHub 在 2s 窗口内合并，result 权威帧优先，消费方不会收到同一帧的两个事件。

## 乱序与重复语义（消费方必读）

DeviceHub 提供 **at-least-once**，不保证恰好一次、不保证全局有序：

1. **重复来源**：WS 主通道与 REST 回补通道会同时覆盖同一帧；MQ 重投；服务重启后游标边界帧重拉（startTs 闭区间）。
2. **乱序来源**：回补按时间升序但 WS 推送实时到达，两者交错；多分区/多实例消费亦乱序。
3. **消费方义务**：
   - 以 `frameId` 为幂等键去重（唯一约束或去重表）；
   - 以 `(tbDeviceId, ts)` 为业务时序基准自行排序，不要用 MQ 到达顺序；
   - 处理失败可重试，重试安全由幂等键保证。
4. **跳过帧**：TB 侧不可解码的帧（如 rule chain 改了存 `decodeStatus:false`）会被 DeviceHub 丢弃并推进游标，消费方看到的是缺口而非毒消息——缺口属预期，不要按连续性告警。
