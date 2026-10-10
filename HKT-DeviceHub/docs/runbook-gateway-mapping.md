# 网关项目映射变更运行手册（备份 → 变更 → 重载 → 验证）

> R-07 配套工单流。适用症状：**设备在 NS 已入网在线，但上行永远到不了 ThingsBoard**（L4 第一嫌疑），
> 或接入预检 ②「网关项目映射」不通过：`项目 {nsProjectId} 无网关映射`。

## 背景与复发原因

设备上行链路：`设备 → NS → TB 网关（OC connector）→ ThingsBoard`。
OC 连接器**只订阅**共享属性里显式配置了 topic 过滤器的 NS 项目：

```
org/{orgId}/project/{nsProjectId}/device/+/dat/up
```

新项目在 NS 侧开通（拿到设备）时，**OC 映射不会自动生成**——两条人工流程，映射步骤常被漏掉。
这是第三次同类复发：148（胶囊）、217（追踪器）、219（2026-09-28 预检发现）。

**预防**：每次新 NS 项目上线，把"加 OC 映射"纳入上线 checklist；平时用缺口巡检提前发现：

```bash
curl -s "$DEVICEHUB/api/v1/channels/mapping-gaps" | jq
# nsAvailable/mappingReadable 均为 true 且 gaps 非空 → 有项目漏映射
# 2026-10-09 起分级：gaps = 活跃缺口（近 7 天有上行，正在丢帧，告警对象）；
# dormantGaps = 休眠缺口（近 7 天无上行，信息级）——这台 NS 是全公司共享的，
# 大量生产测试/出货项目本就不需要 TB 映射，不要按 dormantGaps 数量出警
```

控制台工作台拓扑图 OC 节点与注意事项列表会展示同一份巡检结果（缓存 30s）。

## 自动化路径（控制台一键，2026-09-28 起）

预检 ② FAIL 时页面有「一键自动修复映射」，工作台缺口告警有「修复」，等价于：

```bash
# 先试运行看计划（不写入）
curl -sS -X POST "$DEVICEHUB/api/v1/gateway-mapping/219/apply?dryRun=true" | jq '{addedTopicFilters,newValue}'
# 确认后真正执行：自动备份 → 复制现有条目结构追加 → 写入 → 回读校验 → 网关重载（gateway_restart）
curl -sS -X POST "$DEVICEHUB/api/v1/gateway-mapping/219/apply" | jq
```

映射已在 TB 手工加好、只差重载的场景，直接调独立重载端点：

```bash
curl -sS -X POST "$DEVICEHUB/api/v1/gateway-mapping/reload"
```

**批量修复（2026-10-09 实证，13 个项目一次重载）**：多项目补映射时逐项目 `apply?reload=false`
（每次仍会各自备份 + 回读校验，但不触发重载），全部写入后统一调一次 `reload`，
把「全项目上行数秒中断」从 N 次压到 1 次：

```bash
for p in 17 20 37; do curl -sS -X POST "$DEVICEHUB/api/v1/gateway-mapping/$p/apply?reload=false" | jq '{verifiedAfterWrite,backupPath}'; done
curl -sS -X POST "$DEVICEHUB/api/v1/gateway-mapping/reload"
```

**条目 profile 注意**：自动修复复制「第一条带 `deviceTypeJsonExpression` 的条目」，
该字段是**硬编码 profile 常量**（目前是 `中仪单项电表-OC-配置`）——新条目会把设备建成电表
profile，TB 规则链按电表解码，非电表项目的 `result` 解码会错（`data`/`body`/`dataHex`
原始帧不受影响，保数不保解码）。对解码有要求的项目，写入后在 TB UI 把该条目的
`deviceTypeJsonExpression` 改为对应 profile 再重载；存量正确范例：148→瘤胃胶囊-OC-配置-v2、
217/219→牛羊追踪器-OC-配置-v2。（9-28 批量修复的 77/83/109/130/206 同样背着电表
profile，趁各自项目有活跃设备时一并校正。）

实现要点（`MappingChangeService`，fail-closed）：

- **条目结构不猜**：复制 mapping 数组里现有条目整体结构，仅把 `project/{id}/` 段替换为目标项目（up/down 变体都随模板走）。**模板必须选带 `deviceTypeJsonExpression` 的条目**（代码已优先选取，ddd98a3）：缺该字段的消息在网关重启后设备注册阶段被整条丢弃——2026-09-28 项目 219 从"裸条目"复制后静默两小时即此因；
- **结构异常一律拒绝执行**：attr 缺失 / 值非 JSON / 无 mapping 数组 / 无可复制条目 → 不写任何东西；
- **先备份后写入**：每次真实执行先把变更前的共享属性值落到 `data/oc-backups/`（路径 `devicehub.tb.mapping-backup-dir`）；
- **幂等**：项目已有映射时不写；写入后回读校验并刷新预检用的 30s 缓存；
- **写入后自动重载**：真实写入成功即触发 `gateway_restart` RPC（该网关不会热重载共享属性变更，实测 2026-09-28）；`reload=false` 可跳过重载；
- **总开关**：`devicehub.tb.mapping-auto-fix-enabled=false` 回退为纯人工工单。

重载后 TB 网关重新按共享属性配置订阅全部 topic（数秒中断），以下述第 4 步真实上行到达作为最终验证。

## 附录：网关不再应用配置变更事件与真正生效的重载手段（2026-10-09 实证）

**症状**：OC 共享属性写入 + 回读校验 OK、`gateway_restart` RPC success、网关设备
lastConnect/Disconnect 时间戳证明 MQTT 会话真实重连，但**新增条目的项目就是不来帧**
（总线上实测消息在流、topic 与条目逐字节一致、NS broker 45KB 报文双向通行排除报文上限）。
当日先后尝试：批量 13 条 + restart（仅第 1 条生效）、二次 restart（零生效）、
单写 + restart（零生效）、改投 oc-test1 + restart（零生效）——**`gateway_restart` RPC 在这台
网关上已不能触发连接器配置重载**（9-28 时还可以；期间网关状态已漂移，宿主机未定位 R2）。

**生效手段（已验证）**：摘除再挂回连接器，强制完整重建：

```bash
# 摘除 OC（等 30~60s 让连接器 teardown），再挂回
curl -sS -X POST -H "X-Authorization: Bearer $TB_TOKEN" -H "Content-Type: application/json" \
  -d '{"active_connectors":["oc-test1"]}' "$TB_URL/api/plugins/telemetry/DEVICE/$GW_ID/SHARED_SCOPE"
sleep 40
curl -sS -X POST -H "X-Authorization: Bearer $TB_TOKEN" -H "Content-Type: application/json" \
  -d '{"active_connectors":["OC","oc-test1"]}' "$TB_URL/api/plugins/telemetry/DEVICE/$GW_ID/SHARED_SCOPE"
```

摘除期间 OC 承载的项目上行中断（比 restart 略长，选低峰）。实证效果：挂回后 15 分钟内
25 台新设备创建并开始上报，OC 全量 56 条配置（含此前三重启不生效的 13 条）全部生效，
p231 设备属性 `connectorName=OC` 实锤；老项目（p111 等）toggle 后 6 分钟内恢复上行。

**善后注意**：本次 oc-test1 里留下了 14 条与 OC 重复的条目（双订阅同 topic，同 ts 遥测
覆盖写，幂等无害）。若要清理：备份后把 oc-test1 改回只含 50/86 两条，再做一次上面的
toggle。条目 profile 提醒见前文（自动修复模板背的是 `中仪单项电表-OC-配置`）。

**批量修复防竞态**：一次批量里**逐条写入间隔 ≥2s**、写完等 ≥10s 再重载，重载后必须
用真实上行逐项目验证（本节 §4），RPC success 不算数。


## 前置信息

| 项 | 取值来源 |
|---|---|
| `nsProjectId` | 预检 ② 证据字段，或缺口巡检 `gaps[].nsProjectId` |
| `orgId` | 照抄现有 topicFilter 里的 org 段（NS org 默认 1） |
| 网关设备 ID | `devicehub.tb.gateway-device-id`（默认 `0f7da2b0-e91d-11ef-a8ee-99a8c68f9649`） |

## 1. 备份（勿跳过）

TB UI：设备 → 网关设备 → 「共享属性」→ 复制 key 为 `OC` 的**完整值**，存入工单。
`OC` 的值是一个 JSON 编码字符串，映射条目在 `configurationJson.mapping[].topicFilter`。

REST 备选（token 为 TB 租户级 JWT）：

```bash
curl -s -H "X-Authorization: Bearer $TB_TOKEN" \
  "$TB_URL/api/plugins/telemetry/DEVICE/$GW_ID/values/attributes/SHARED_SCOPE" \
  | tee "oc-backup-$(date +%Y%m%d-%H%M)-ticket-工单号.json"
```

## 2. 变更

编辑 `OC` 共享属性：在 `configurationJson.mapping[]` **复制一条现有条目**（保持字段结构一致，
现有条目若同时含 `.../dat/down` 就一并复制），把 `project/` 段替换为目标项目号：

```json
{ "topicFilter": "org/1/project/219/device/+/dat/up" }
```

**只增不删**，其余字段一律原样保留。UI 编辑长 JSON 字符串容易误伤时，用 REST 整体回写
（注意是全量替换，必须基于备份 JSON 修改后再 PUT）：

```bash
curl -s -X POST -H "X-Authorization: Bearer $TB_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"OC": "<修改后的完整 JSON 字符串>"}' \
  "$TB_URL/api/plugins/telemetry/DEVICE/$GW_ID/SHARED_SCOPE"
```

## 3. 重载

让网关重新加载 OC 连接器配置，记录重载时间。重载期间全项目上行会短暂中断，选择业务低峰执行。

**实测结论（2026-09-28）**：这台 TB 网关**不会**因共享属性变更热重载连接器——映射写入后（甚至同值重写）订阅集纹丝不动。必须真正重启：

```bash
# 首选：TB RPC 全网关重启（实测 2026-09-28 成功，秒级恢复）
curl -sS -X POST -H "X-Authorization: Bearer $TB_TOKEN" -H "Content-Type: application/json" \
  -d '{"method":"gateway_restart","params":{}}' \
  "$TB_URL/api/rpc/twoway/$GW_ID"
# 注意：connector_reboot 在此网关上不可用（active_connectors 里明明有 OC/oc-test1，
# 但 RPC 一律报 "connector not found in available connectors"——注册表与共享属性已脱节），
# 不要被它耗时间，直接 gateway_restart。
```

**若 gateway_restart 后新条目仍不来帧**（2026-10-09 起该 RPC 已不重载配置，只剩会话重连）：
改用文末附录的 `active_connectors` 摘除/挂回法，那才是当前实证有效的重载。

**重启生效的验证方法**（不必等目标设备）：
1. 找任一**已映射项目**设备在重启后于 NS 的 `last_seen`（NS 设备列表 API 有此字段，注意 NS API 走 80 端口，`:8080` 是 404）；
2. 查该设备 TB 遥测出现同时间戳的帧 → 连接器已按新配置重订全部 topic；
3. 目标项目的设备下一帧自然落入 TB。

## 4. 验证（四项全过才算完成）

1. **预检**：`GET /api/v1/devices/preflight?devEui={该项目任一设备}` → ② PASS；
2. **NS 侧确认有上行**：注意 `nsFrameCountUp=0` 的设备是还没发过上行，先等/触发一帧再验；
3. **TB 到达**：TB 设备最新遥测（`result`/`dataHex`）出现新时间戳；
4. **巡检归零**：`GET /api/v1/channels/mapping-gaps` 中该项目从 `gaps` 消失。

## 5. 留档

工单记录：变更前 diff（备份 vs 新值，建议 `jq` 精确到 mapping 数组下标）、重载时间、四项验证证据、执行人。
