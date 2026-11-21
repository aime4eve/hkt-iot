# 设备下行命令模块（phase 2）设计规格

> **追溯** | [traceability-matrix.md](../../../docs/traceability-matrix.md)
> - **源需求**：smart-livestock 仓 issue [#22「下发指令设置追踪器上报数据时间间隔」](http://172.22.1.123/hkt-smart-agentic/l3/smart-livestock/-/issues/22)（工单 ①，2026-11-21 用户裁决 D1=C：DeviceHub 难点先行；D2=B：FPort 可配+真机探测定案；D3=B：一期含双时段）。
> - **验收标准**：见本文 §9。
> - **上下文**：`docs/roadmap-commands.md`（二期设计骨架，本 spec 将其落地为可实施契约）；固件实据 `02-firmware/4.LORAWAN牛羊追踪器/LoRaWAN_Animals_Track`。
> - **实现产出**（关闭时补）：

## 1. 目标与范围

**目标**：DeviceHub 开放通用下行命令通道，使业务项目（首个消费方 smart-livestock）可对 LoRaWAN Class A 设备异步下发配置命令并追踪其状态至设备 ACK。

**里程碑 M0（先行，本模块第一个交付物）**：下行链路真机可行性验证——TB RPC → OC connector → NS → 设备 RX 窗口 全链路从未实测，OC connector 的 RPC 下行映射是否可用是整个工单的最大不确定性（D1=C 裁决的直接原因）。M0 结果决定走主路径（TB RPC）还是备选路径（DeviceHub 直连 NS enqueue）。

**范围内**：
- M0 探测（§3）：FPort 定案（D2=B）+ OC connector RPC 映射实测；
- 命令 REST API（§4，对 ② 冻结的契约）；
- `device_commands` 持久化与状态机（§5）；
- capabilities 下行能力校验（§6）；
- 延迟调度与每设备串行（§7）；
- 命令编码器：`setTimeSync`(0x80)、`setReportConfig`(0x39)（帧格式 §8）。

**范围外**：
- 设备端固件改动（固件能力已具备，零改动）；
- "已生效"判定（0x39 回读比对）——属 livestock 侧职责（§5.3 边界声明）；
- 业务语义（谁、对哪台牲畜、为什么改间隔）——业务项目自己的领域；
- 批量命令编排（错峰策略仅预留 `scheduled_at`，不建批量 API）。

## 2. 链路总览

```
livestock-server ──POST /api/v1/devices/{id}/commands──▶ DeviceHub CommandController
    → application/command 编排：capabilities 校验 → 帧编码 → device_commands 入库(PENDING)
    → application/scheduling 调度（每设备串行）→ TbClient.sendRpcOneWay（M0 后若 TB 路径不通 → NsClient 直连 enqueue 备选）
    → ThingsBoard server-side RPC → IoT Gateway (OC connector, RPC 下行映射) → NS (ChirpStack downlink queue)
    → 设备下次上行后的 RX1/RX2 窗口接收 → 设备回 ACK 上行帧
    → DeviceHub 遥测通道（既有 MQ/TELEMETRY_FRAME 消费）识别 0xFF ACK → SENT→ACKED
```

## 3. 里程碑 M0：下行链路真机探测（先行交付物）

**目的**：一次性回答两个黑盒问题——① OC connector 的 RPC→NS 下行映射是否真实可用（历史上 RPC success≠生效，`runbook-gateway-mapping.md:100-110`）；② 模组 LSD4WN 对下行 FPort 是否过滤（固件 MCU 层不校验端口，只认同步头 `68 6B 74`）。

**前置**：
- dev 环境（TB=172.22.3.105 / NS=172.17.201.15:8080 / DeviceHub=172.17.10.206:8080）就绪；
- 一台 GAT-100 真机已注册（`registered_devices` 有行、`tb_device_id` 已关联、project=LIVESTOCK）；
- 探测命令用 **0x80 时间同步**：无副作用（只重设 RTC），且 header 置需应答位后设备回 ACK，是理想的链路探针。

**步骤**（预计 1 小时内完成，主要是等上报周期开 RX 窗口）：
1. `capabilities` 写入 `{"downlink":true,"deviceClass":"A","fPort":10,"commands":["setTimeSync","setReportConfig"]}`；
2. 经 DeviceHub 内部冒烟通道（不新增对外 API，用 `@SpringBootTest` 集成测试或 dev 临时运维端点）调 `TbClient.sendRpcOneWay(tbDeviceId, {"method":"downlink","params":{"fPort":10,"dataHex":"...0x80帧...","confirmed":false}})`——**method/params 命名即 M0 要定案的第一个契约点**，以 OC connector 实际配置为准；
3. 0x80 下行帧（13 字节）：`68 6B 74 01 <seq> 80 <YY MM DD HH MM SS>`（bit0=1 请求 ACK）；
4. 等设备下次上行（默认周期 30min）后观察四个证据点：TB RPC 投递状态 / NS 设备 queue 计数变化（`NsClient.fetchDeviceDetail` 的下行计数）/ 设备 RTC 是否被同步（下一帧时间字段）/ **ACK 帧 `68 6B 74 00 <seq> FF FF` 是否出现在遥测通道**；
5. fPort=10 无 ACK → 改 `fPort=2` 重复一轮（两轮都在无副作用命令上试，不消耗配置语义）；
6. 结论写入 `docs/runbook-downlink.md`（新建）：可用路径、生效 FPort、method/params 契约、时延实测；FPort 定案同步进 capabilities 种子数据。

**失败分支决策点**：TB RPC 路径不通（OC connector 无 RPC 映射或持续假成功）→ 启用备选路径：`NsClient` 已有 ChirpStack org API 基础（`lora_wan/device/list|detail`），补一个 enqueue 方法直连 NS 设备队列。此分支改变 §2 链路中部但不影响 §4 API 契约（对 ② 透明）。

## 4. REST API 契约（对 ② 冻结）

资源路径沿用既有占位：`/api/v1/devices/{deviceId}/commands`（deviceId = `registered_devices.id`）。

| 方法 | 路径 | 语义 | 请求体 | 响应 |
| --- | --- | --- | --- | --- |
| POST | `/api/v1/devices/{deviceId}/commands` | 创建并排队命令 | `{"command":"setReportConfig","confirm":true,"payload":{...}}` | `202 {"commandId":123,"status":"PENDING"}`；非法值/能力不符 → `422` + 错误码 |
| GET | `/api/v1/devices/{deviceId}/commands?status=&limit=` | 该设备命令列表（倒序） | — | `200 [{commandId,command,status,createdAt,sentAt,ackedAt,failureReason},...]` |
| GET | `/api/v1/commands/{commandId}` | 单条详情 | — | `200` 同上单对象 + `payload` 回显 |
| POST | `/api/v1/commands/{commandId}/cancel` | 取消 | — | PENDING→`200 CANCELLED`；已 SENT→`409`（Class A 窗口已占用不可撤） |

**约束**：
- `command` 一期枚举：`setTimeSync` / `setReportConfig`；必须 ∈ 该设备 `capabilities.commands` 白名单，否则 422。
- `payload` 按 command 由 DeviceHub 校验并编码为下行帧（§8）——**业务侧传结构化字段，不传裸 hex**（编码责任单一归属 DeviceHub，杜绝各业务重复实现组包且口径漂移）。
- `confirm=true` 时下行 header bit0=1（设备回 ACK）；`setReportConfig` 强制 confirm=true。
- 幂等：同设备存在非终态命令时 POST 返回 `409 COMMAND_IN_FLIGHT`（每设备串行，§7）。
- 错误响应双语 message（DeviceHub 现无 i18n 框架，一期用错误码 + 英文 message，错误码枚举进 `telemetry-event-contract.md` 同级的新契约文档 `command-api-contract.md`，随 plan 一并落）。

## 5. 命令状态机与持久化

### 5.1 状态机（沿用 roadmap，四态 + CANCELLED）

```
PENDING ──调度发出──▶ SENT ──上行ACK帧(0xFF)时间窗内──▶ ACKED（终态）
   │                    ├── TB/NS 拒绝 ──▶ FAILED（终态）
   │                    └── 超过确认窗口未收 ACK ──▶ TIMEOUT（终态）
   └── 人工取消 ──▶ CANCELLED（终态）
```
- 终态不复活；重试 = 新建命令（审计链完整）。
- TIMEOUT 窗口 = `sent_at + 2×max(设备当前上报间隔, 30min)`，可配置。

### 5.2 DDL（`V2__device_commands.sql`）

```sql
CREATE TABLE device_commands (
    id             BIGSERIAL PRIMARY KEY,
    device_id      BIGINT NOT NULL REFERENCES registered_devices(id),
    command        VARCHAR(64) NOT NULL,
    payload        JSONB NOT NULL DEFAULT '{}',
    status         VARCHAR(16) NOT NULL DEFAULT 'PENDING'
                   CHECK (status IN ('PENDING','SENT','ACKED','FAILED','TIMEOUT','CANCELLED')),
    correlation_id VARCHAR(64),        -- 下行 seq + 时间窗弱关联键（见 5.3）
    scheduled_at   TIMESTAMPTZ,        -- NULL=立即
    sent_at        TIMESTAMPTZ,
    acked_at       TIMESTAMPTZ,
    failure_reason VARCHAR(256),
    created_at     TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at     TIMESTAMPTZ NOT NULL DEFAULT now()
);
CREATE INDEX idx_device_commands_device ON device_commands (device_id, created_at DESC);
-- 每设备串行的物理保证：任一设备同时至多一条非终态命令
CREATE UNIQUE INDEX uk_device_commands_active
    ON device_commands (device_id) WHERE status IN ('PENDING','SENT');
```

### 5.3 ACK 关联与职责边界（重要）

固件 ACK 帧 `68 6B 74 00 <seq> FF FF` **不携带命令标识**（固件实据 `communicate.c:302-320`）。因此：
- DeviceHub 的 SENT→ACKED 用 `correlation_id = <下行seq>` + 时间窗匹配该设备上行遥测中的 0xFF 帧——**弱关联**，同一时间窗内只有一条在途命令时（§5.2 唯一索引保证）退化为可靠；
- **"已生效"（APPLIED）不设状态**：0x39 回读值 = 下发值的比对属业务语义，由 livestock 侧消费遥测自行判定（#22 验收标准 2 的三态第三态）。此边界写进 API 契约文档，防止 ② 误等一个永不出现的 DeviceHub 状态。

## 6. capabilities 下行能力校验

`registered_devices.capabilities`（JSONB，`V1__init.sql:7`）下发前校验：
```json
{"downlink": true, "deviceClass": "A", "fPort": 10, "commands": ["setTimeSync", "setReportConfig"]}
```
- `downlink` 缺失或 false → 422 `DEVICE_NOT_DOWNLINK_CAPABLE`；
- `command` ∉ `commands` → 422 `COMMAND_NOT_SUPPORTED`；
- `fPort` 缺省默认 10（D2=B 裁决，M0 定案后写种子）；
- `deviceClass` 仅信息性（当前全系 Class A），为未来 Class C 预留调度语义。

## 7. 调度与串行

- `application/scheduling/`：DB 轮询 `status='PENDING' AND (scheduled_at IS NULL OR scheduled_at<=now())`，行级锁（`FOR UPDATE SKIP LOCKED`）抢占；多实例部署时上 ShedLock（与 roadmap 一致，单实例 dev 可先不做，代码留开关）。
- 每设备串行由 §5.2 唯一索引物理保证，调度器只管触达。
- 发送失败重试：TB/NS 瞬时错误退避重试 3 次（30s/2m/10m），仍败 → FAILED。
- 批量错峰：一期无批量 API；未来批量走 `scheduled_at` 随机抖动，避免 NS duty cycle 风暴（roadmap 已述，此处只留字段不留实现）。

## 8. 命令编码（帧格式，固件实据）

同步头 `68 6B 74` + special_type（bit0=需应答）+ packSyncNumber(seq)，随后 TLV（type + 定长 value，大端）。

| command | 帧结构 | 校验（DeviceHost 侧前置拦截，早于固件的 1~1440 丢弃保护） |
| --- | --- | --- |
| `setTimeSync` | `68 6B 74 01 <seq> 80 YY MM DD HH MM SS`（13B） | 时间由服务器生成（UTC+8 墙钟，与固件 `systime` 语义一致），不接受业务传值 |
| `setReportConfig` | `68 6B 74 01 <seq> 39 <runMode> <i1:u16> <s1h> <s1m> <e1h> <e1m> <i2:u16> <s2h> <s2m> <e2h> <e2m> <idle:u16>`（21B） | runMode∈{0,1}；间隔统一 **10~1440**（平台保守口径，固件为 1~1440——差异记录：固件按 1~1440 校验后接受，但 <10min 严重折损电池寿命承诺口径，平台不提供入口）；runMode=1 时段字段 HH<24、MM<60，时段 1/2 不得重叠 |

双时段（runMode=1）为一期范围（D3=B 裁决）。

## 9. 验收标准

**M0（先行单独验收）**：
1. 真机经全链路收到 0x80 且 RTC 被同步，或明确记录 TB 路径不可用并给出 NS 直连备选实测结论；
2. 生效 FPort 定案并写入 capabilities 种子与 `runbook-downlink.md`；
3. method/params 契约（OC connector 映射的真实要求）成文。

**模块**：
4. §4 四个端点行为与契约一致（含 409 COMMAND_IN_FLIGHT、422 校验、409 取消已发命令）；
5. 状态机全路径单测覆盖（含 TIMEOUT 窗口、重试退避）；
6. `device_commands` DDL 经 Testcontainers 全新库跑通（含 uk_device_commands_active 并发插入只成功一条）；
7. 编码器对 §8 两命令的组包有字节级断言单测；
8. `./mvnw verify` 通过；MR 经 /approve 合并，CI 绿。

## 10. 风险与开放点

| 风险 | 缓解 |
| --- | --- |
| OC connector RPC 映射不可用/假成功（历史教训） | M0 先行（D1=C 裁决核心）；证据四点判定不只看 RPC success；备选 NS 直连 |
| ACK 弱关联 | 每设备单命令在途（唯一索引）+ 时间窗；APPLIED 不进 DeviceHub 职责 |
| FPort 黑盒 | D2=B：可配 + M0 一次定案 |
| 下行风暴/duty cycle | 一期无批量；`scheduled_at` 预留 |
| DeviceHub REST 无鉴权（现状，内网部署） | 开放点：一期沿用内网边界（206 端口仅内网可达），二期随管理控制台补内部令牌——**需在 ② 接入时确认网络面** |
| `project` CHECK 只有 LIVESTOCK/PARKING | 不涉改表；PARKING 未来复用同一命令通道 |
