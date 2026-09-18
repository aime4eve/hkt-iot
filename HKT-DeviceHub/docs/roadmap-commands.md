# 二期路线图：下行命令 + 设备调度

本期（一期）只交付占位：`TbClient.sendRpcOneWay/sendRpcTwoWay` 已封装仅供内部冒烟，`CommandController` 对 `/api/v1/devices/{id}/commands/**` 一律返回 501。本文档锁定二期设计要点，实施时按此展开。

## 下行链路

```
业务项目 → DeviceHub CommandController（二期开放，501 → 200）
        → application/command 编排
        → TbClient.sendRpcOneWay/TwoWay（POST /api/rpc/{oneway|twoway}/{deviceId}）
        → ThingsBoard server-side RPC
        → IoT Gateway (TB connector, RPC 下行映射)
        → NS (ChirpStack downlink enqueue)
        → LoRaWAN 设备
```

约束：

- **Class A 下行窗口**：LoRaWAN Class A 设备只在每次上行后的 RX1/RX2 窗口接收，下行必须先进入 NS 队列等待下一帧上行触发。因此命令是**异步**语义——TB twoway RPC 的 timeout 只覆盖 TB→网关段，不代表设备已执行。
- `registered_devices.capabilities`（JSONB）承载每设备的下行能力声明，例如：
  `{"downlink": true, "deviceClass": "A", "fPort": 2, "commands": ["setInterval", "reboot"]}`。
  编排层先查 capabilities 再决定路由与参数校验；`downlink:false` 或缺失的设备直接拒绝命令。

## 命令状态机

```
PENDING → SENT → ACKED
              ↘ FAILED（TB/网关拒绝、设备不支持）
              ↘ TIMEOUT（超过确认窗口未收到 ACK，Class A 默认等下一次上行）
```

- `PENDING`：已入库未下发（等调度/等上行窗口）。
- `SENT`：TB RPC 已受理。
- `ACKED`：设备侧确认（上行帧携带的命令回执，经 DeviceHub 遥测通道回流关联）。
- `FAILED` / `TIMEOUT`：终态，可人工重试生成新命令（不原地复活，保持审计链）。

## 数据模型（二期新增）

```sql
device_commands(
  id BIGSERIAL PK,
  device_id BIGINT NOT NULL REFERENCES registered_devices(id),
  command VARCHAR(64) NOT NULL,
  payload JSONB NOT NULL DEFAULT '{}',
  status VARCHAR(16) NOT NULL DEFAULT 'PENDING'
         CHECK (status IN ('PENDING','SENT','ACKED','FAILED','TIMEOUT')),
  correlation_id VARCHAR(64),        -- 关联设备上行 ACK 帧
  scheduled_at TIMESTAMPTZ,          -- 调度触发时间，NULL=立即
  sent_at TIMESTAMPTZ,
  acked_at TIMESTAMPTZ,
  failure_reason VARCHAR(256),
  created_at/updated_at TIMESTAMPTZ
)
```

ACK 关联：设备上行 ACK 帧经 TelemetryFrame 回流（properties 内携带命令回执字段），`application/command` 消费本服务自己的 MQ 事件做状态推进——复用一期契约，不新增链路。

## 调度模型

- `application/scheduling` 包：基于 `scheduled_at` 的延迟调度（DB 轮询 + 行级锁抢占，或多实例时引入 ShedLock）；批量命令支持按项目/按设备组分片错峰，避免下行风暴打爆 NS duty cycle。
- 每设备串行：同一设备同时只有一条非终态命令（Class A 窗口稀缺），后续命令排队。

## 代码落点

| 模块 | 位置 | 说明 |
|------|------|------|
| 命令编排 | `application/command/` | 状态机、capabilities 校验、TB RPC 调用、ACK 关联消费 |
| 调度 | `application/scheduling/` | 延迟/批量调度 |
| 持久化 | `domain/model/DeviceCommand` + `V2__device_commands.sql` | 见上 DDL |
| API | `interfaces/CommandController` | 去掉 501，开放下发/查询/取消 |
| 客户端 | `TbClient.sendRpcOneWay/TwoWay` | 一期已就位，二期直接启用 |
