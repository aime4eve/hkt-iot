# NS 直连 ThingsBoard 备份遥测通道：论证与演进方案

> 2026-09-29 立项（方案定稿）。回答两个问题：**为什么**需要一条不经 OC 网关映射的 NS→TB 直连通道；**如何**分阶段落地。
> **Phase 0 spike 已于同日执行，四项出结论、24h 满窗待终审，见 §8。**
> 关联文档：`runbook-gateway-mapping.md`（现行桥接运维）、`integration-cutover.md`（下游收口）、`telemetry-event-contract.md`（帧契约）。
> 文中行号基于 main@aef978b；所有"实证"来自 2026-09-28 的 219 事件复盘与本仓库代码。

## 0. 结论先行

**现行上行链路是一条无人看管的独木桥；值得在 DeviceHub 里修一条常开的并行通道，让 NS 直连 TB——断桥时业务零丢帧，正常时靠现有去重机制零感知，总代价是 TB 遥测存储约翻倍。**

- 推荐：**DeviceHub 内嵌备份通道（路线 B）+ 热备双跑（常开）**。
- 不推荐：NS 原生集成（把映射漂移换成了 token 同步漂移，还受制于定制 NS 的控制权）；冷备待命（恰恰在最需要它的故障窗口内丢帧）。
- 节奏：Phase 0 spike（0.5~1 天）→ Phase 1 MVP（试点项目双跑）→ Phase 2 故障检测与 runbook → Phase 3 转正裁决。每阶段回退 = 一个配置开关。
- 本文只做论证与方案，未写任何代码。

## 1. 现状：唯一上行路径是一条独木桥

### 1.1 链路与角色

```
LoRaWAN 设备 → NS（ChirpStack-compatible，172.17.201.15）
   MQTT: org/{org}/project/{nsProjectId}/device/+/dat/up
   ▼
「通用接入网关」= TB IoT Gateway + OC 连接器（外部组件，宿主机至今未定位）
   只订阅 TB 共享属性 OC.configurationJson.mapping[] 里显式列出的项目 topic
   ▼
ThingsBoard 3.8.0（172.22.3.105）—— 遥测面 + 治理面双重角色
   ▼ DeviceHub（WS 主通道 + REST 回补，TB 访问唯一收口）
RocketMQ DEVICEHUB_TELEMETRY_FRAME → smart-livestock / smart-parking
```

证据：`README.md:7-9`（链路图）、`integration-cutover.md:8-21`（收口原则）、映射数据结构与配置键 `application.yml:39,42`（gateway-device-id / mapping-attr-key）。

两个关键事实：

1. **NS 全量上总线，筛选是桥接的职责**：2026-09-28 用 OC broker 凭据订阅 `org/1/project/#`，3 分钟内十来个项目（含未映射项目）实时在流——总线旁听实证（219 复盘记录）。
2. **TB 是上行遥测唯一入口**：DeviceHub 只从 TB 读；唯一例外是 parking 的 blade 通道（与 TB 无关，`integration-cutover.md:47`）。

### 1.2 桥接的四类结构性风险（全部有实证）

| # | 风险 | 实证 |
|---|------|------|
| R1 | 按项目显式订阅 + 手工映射 → **静默丢帧** | 148/217/219 三次同类复发（`runbook-gateway-mapping.md:16`）；219 双探针：总线 966 条在流、TB 零接收——桥接丢弃 100% 实锤 |
| R2 | 中心化单实例、**宿主机不明** | 候选宿主机 123/86/171/206 全部排除、TB 租户内仅一台网关设备（219 复盘）；指纹在案待运维认领 |
| R3 | 配置不热重载，`gateway_restart` 全局中断数秒，且 RPC success ≠ 生效 | `runbook-gateway-mapping.md:100-110`（实测不热重载、connector_reboot 已坏）；219 事件两次轻信 success 回包（假重启） |
| R4 | 条目结构脆弱：缺 `deviceTypeJsonExpression` 的条目在网关重启后整条丢弃 | 219 从"裸条目"复制后静默两小时（`runbook-gateway-mapping.md:46`，commit ddd98a3） |

### 1.3 已有缓解为什么还不够（治标不治本）

2026-09-28 已上线：映射缺口巡检（`GET /api/v1/channels/mapping-gaps`）、一键修复（`POST /api/v1/gateway-mapping/{id}/apply`，备份→复制模板→写入→回读→自动重载）、独立重载端点（`GatewayMappingController.java:30,40`）、映射备份卷。

但这四件事都没改变独木桥的属性：

- 巡检只能发现"**没映射**"，发现不了"**映射了但桥接进程用旧名单/旧结构丢帧**"——219 正是后者（纸面配置对、进程用旧名单）。
- 修复动作本身要 `gateway_restart`（R3），全项目中断数秒。
- 丢帧检测未成体系：处置单分诊有雏形（`DisposalService.java:100-132`：NS `fCntUp>0` 而 TB 无数据 → PLATFORM 类），但只覆盖已注册设备的静默场景，不是项目级持续对账。
- **桥接进程级故障（宕机、宿主机失联）没有任何映射修复能救**——只有第二条通道能救。且 MQTT 无保留消息，未订阅即丢失，故障发生到被发现窗口内的帧永远找不回来。

## 2. 直连可行性：三条部署内证据 + 一个关键设计红利

### 2.1 证据一：这套 NS 上直推跑通过（旧代际链路）

设备 1264f1/1264fa 的 TB 遥测 key 是 `body`+`data`（base64），body 为 `{channel:'LoRaWAN', ts, fPort, object, rxInfo}`——NS 直推 TB 的原始格式（2026-09-16 归因笔记：新设备 TB 有遥测但开通 preflight 不过的归因）。**证明 NS 侧存在能直推 TB 的集成能力，且本部署实际用过。**

它同时暴露直连的三个契约缺口（= 主要工作量所在）：

1. ts 秒级浮点被当毫秒存 → TB 显示 1970（时间单位必须归一化）；
2. `object:{}` 空 → NS 端未配解码器，解码责任缺失；
3. profile「动物追踪器-规则链-v1-kafka 增强版」不在白名单 → 平台不认。

**结论：直连 ≠ 免费，但"能通"已被本部署验证，不是纸上假设。**

### 2.2 证据二：总线通配订阅在凭据/ACL 层面可行

219 复盘已用 OC broker 凭据成功订阅 `org/1/project/#`（通配）并持续 35+ 分钟看到全项目在流。约束与前提：

- 必须用**独立 clientId**：现网桥接 clientId 为 `gw_pLoh7THJzthWrYDLk2mE_2`，同 clientId 重连会互踢会话。
- `$SYS/#` 被 ACL 拒绝说明 broker 有 ACL，但业务 topic 通配未被拦截。
- QoS1 + cleanSession=false（离线消息排队）是否被允许 → Phase 0 ①验证；被拒则评估 QoS0 的丢帧率。

### 2.3 证据三（设计红利）：现有契约天然吸收双通道重复

这是本方案成本可控的核心，三条机制全部已存在于代码：

| 机制 | 出处 | 对双跑的意义 |
|---|---|---|
| `frameId = UUIDv3(tbDeviceId + ":" + ts)`，与通道、到达顺序无关 | `TbFrameNormalizer.java:147-151`、`telemetry-event-contract.md:33-43` | 同一物理帧无论从哪条通道来，frameId 相同 → 消费方按 frameId 幂等即可，无需新建去重设施 |
| result/dataHex 2s 同帧窗口合并 | `TbFrameNormalizer.java:38,89-95,130-138` | 备份通道写的 dataHex 若与桥路径 result 帧 ts 差 ≤2s，被静默合并——正常期业务零重复 |
| dataHex 兜底帧是一等公民 | `telemetry-event-contract.md:26`（`dataHex`：仅当 2s 窗口内无权威 result 帧时存在）+ 第 3/4 节 | 桥丢帧时，备份通道的 dataHex 独立成兜底帧照常下发——**业务数据不丢，只是业务字段降级** |

**硬前提（Phase 0 必验，不达标不 Go）**：两条通道对同一物理帧写入 TB 的 ts 差必须 ≤2s。桥路径的 TB ts 由网关上报（源头是 NS 消息 ts），备份路径的 ts 由我们归一化写入——两者口径必须一致。若 ts 差超出 2s 窗口，双跑期会产生 frameId 不同的重复帧，下游业务唯一键（livestock `(device_id, report_time)`）兜不住。不达标时的处理：先修 ts 归一化口径；仍不达标则热备不成立，回退冷备对比。

### 2.4 为什么不能指望"继续加固桥接"替代直连

- 桥接是外部组件且宿主机未定位（R2）——代码层加固无从下手，只能隔着 TB 共享属性遥控。
- 巡检+一键修复已把"发现+修复"做到最好，但**发现时窗口内丢的帧找不回来**。
- 加固解决 R1/R4（配置类故障），对 R2/R3（进程/宿主机故障、全局重启）无解。直连通道对四类风险**全部**有兜底。

## 3. 实现路线对比

| 维度 | A. NS 原生 TB 集成 | **B. DeviceHub 内嵌备份通道（推荐）** | C. 官方 TB IoT Gateway 镜像自部署 |
|---|---|---|---|
| 原理 | ChirpStack 应用级启用集成，NS 直推 TB MQTT | DeviceHub 加 MQTT 订阅模块：通配订阅 → 匹配注册表 → TB REST 直写 dataHex | 自部署 thingsboard-gateway 容器，配置进 git |
| 映射漂移 | 换成 per-app 启用 + per-device `ThingsBoardAccessToken` 变量同步（ChirpStack v3 官方机制）——**新的漂移点，恰是要治的病** | **结构性消灭**：通配订阅 + 注册表匹配，无逐项配置 | 缓解：配置进 git 可评审；但 deviceType 为常量，仍需逐项目条目 |
| 身份契约 | 设备按 token 对齐，TB 侧行为未知 | 复用 registered_devices 的 tbDeviceId，**天然同设备同 profile**，preflight/巡检语义不变 | 需复刻网关协议 deviceName/deviceType |
| 解码 | 依赖 NS codec（定制 NS 能力未知） | 走 dataHex 兜底帧（契约内建）；若 TB 规则链对 REST 写入触发解码则自动升格（Phase 0 ②验证） | 与现行桥完全同语义 |
| 新增依赖 | NS 侧改造（定制 NS，控制权不明） | DeviceHub 加 paho 依赖（现无任何 MQTT 依赖，`pom.xml:140` 仅 rocketmq） | 新容器 + 宿主机协调 + 独立监控面 |
| 可观测性 | NS 侧 | 控制台直接挂卡（DeviceHub 本就是控制台宿主） | 需自建 |
| 故障隔离 | 独立于 DeviceHub | 与 DeviceHub 共命运——但 DeviceHub 挂了下游本无消费者，隔离无增益 | 独立进程 |
| 改动面 | NS 配置 + NS 侧 per-device 变量 | 一个模块 + 控制台卡片，默认关 | 一个容器 + 配置仓库 |

### 推荐 B 的完整数据流

```
NS MQTT org/1/project/+/device/+/dat/up（QoS1，独立 clientId，备份专用凭据）
  → DeviceHub BackupChannelService（devicehub.backup-channel.enabled，默认 false）
     ├─ 解析消息：devEui / nsProjectId / data(base64→hex) / ts / rxInfo
     ├─ 匹配 registered_devices(ACTIVE) → tbDeviceId；未知设备只计数不落库
     ├─ ts 归一化（单位口径按 Phase 0 ④ 校准，单测锁定）
     └─ TB REST：POST /api/plugins/telemetry/DEVICE/{tbDeviceId}/TIMESERIES
           body: [{"ts": <epoch ms>, "values": {"dataHex": "..."}}]   ← 显式 ts，非服务端时间
  → TB 设备 timeseries（与桥路径同一设备，消费链无感知）
  → DeviceHub 现有消费链自动拾取：
     与桥 result 同 ts（≤2s）→ 2s 窗口合并，零重复；
     桥缺帧 → dataHex 兜底帧下发 RocketMQ，业务保数降级
```

三个设计取舍说明：

1. **为什么写 `dataHex` 而不是自建 key**：复用契约既有语义与消费链，不引入第二套词表（同类教训见知识库 40-Pitfalls「source 词表不一致」）。
2. **为什么 TB REST 直写而不是再走 MQTT 网关协议**：DeviceHub 已持有租户级 TB REST 通道（TbClient，认证/重试/401 重登现成）；REST 可携带显式 ts，从根上规避旧代际的 1970 类时间 bug。
3. **备份通道刻意不做的事**：解码（那是 TB 规则链/NS codec 的职责，备份通道不抄解码逻辑）、下行、未知项目/未知设备落库（只计数——这个计数本身是增量价值：DeviceHub 第一次有了"全总线视角"的流量观测）。

## 4. 运行语义：热备双跑 vs 冷备

| 维度 | 热备双跑（推荐） | 冷备待命 |
|---|---|---|
| 断桥时丢帧 | **零**（通道常开；219 的 966 条能全保） | 「故障发生→被发现→被启用」窗口内全丢（MQTT 无保留，219 场景丢大头） |
| 正常期成本 | TB 遥测存储约翻倍（同帧 result + dataHex×2）；TB UI 同帧多行 | 近零（仅巡检/对账查询） |
| 人工介入 | 无（故障自愈，无切换动作） | 需一次启用动作，且依赖"故障已被发现" |
| 去重 | 依赖 §2.3 机制（ts 对齐硬前提） | 无重复问题 |
| 恢复闭环 | 持续对账即可 | 桥修复后需关闭备份并验证，防双跑状态残留 |

推荐热备双跑。理由：本方案的存在意义就是"219 那 966 条不再丢"，冷备恰恰在最需要它的故障窗口内丢帧；存储翻倍对当前遥测量级（分钟级上报、帧体小）可接受；TB UI 同帧多行可解释（dataHex 与 result 本就是同一帧的两个 key）。过渡路径：Phase 1 先在试点项目热备一个完整观察周期（含至少一次 `gateway_restart` 演练），再决定全量推开与最终语义定稿。

## 5. 演进方案（四阶段）

### Phase 0 — 可行性 spike（0.5~1 天；只读 + 试点写，不动现网桥）

| # | 验证项 | 方法 | 通过标准 |
|---|---|---|---|
| ① | 通配订阅长连稳定性 | 独立 clientId 订阅 `org/1/project/#`，QoS1 + cleanSession=false，跑 24h | 不被踢；离线期消息能补收。若 ACL 拒持久会话/QoS1，记录限制并评估 QoS0 丢帧率 |
| ② | TB REST 直写落库 + 规则链是否触发解码 | 对试点设备 `POST /api/plugins/telemetry/DEVICE/{id}/TIMESERIES` 写一条历史 dataHex | 落库成功；观察是否生成 result。触发→备份帧自动升格权威帧（最佳）；不触发→接受"保数不保解码"的备份语义，结论写入本文档 |
| ③ | 双通道 ts 对齐 | 同一物理帧：桥路径 TB ts vs 备份路径归一化 ts，取多帧样本 | 差值 ≤2s（硬门槛；不达标先修 ts 口径，仍不达标则热备不成立） |
| ④ | NS 消息 ts 单位 | 多项目消息样本核对 ts 字段类型/量纲 | 确定归一化公式并以单测锁定（旧代际秒存毫秒的前科必须在此终结） |
| ⑤ | （顺带，非阻塞）桥接宿主机 | 向运维交底指纹（219 复盘在案：双 1883 连接 + clientId） | 拿到宿主机与容器名，消除 R2 |

产出：spike 结论回填本文档 §2.3/§4，Go/No-Go。

### Phase 1 — 备份通道 MVP（DeviceHub 内嵌，试点项目）

- 后端：paho 依赖；`BackupChannelService`（订阅 → 匹配 → 限流 → REST 写）；配置键 `devicehub.backup-channel.enabled`（默认 false）+ 项目白名单（先 1 个试点项目）；失败重试与丢弃日志。**备份通道自身不得成为新的静默丢弃点：所有丢弃必须计数并进控制台。**
- 控制台：工作台新增「备份通道」卡——运行状态/订阅确认/今日帧数/最近帧时间/未知设备计数/开关。
- 对账：缺口巡检扩展为"投递缺口"（NS `fCntUp` vs TB 帧数差），桥健康时该指标应≈0；越限即告警。
- 验收：试点项目双跑 ≥1 周；期间至少 1 次 `gateway_restart` 演练——演练窗口业务入账**零丢帧、零重复**（frameId 口径对账）；正常期对账差≈0。
- 回退：开关关闭即回到现状，无代码回滚。

### Phase 2 — 故障检测与 runbook

- 「NS 有上行但 TB 无帧」体系化：处置单分诊（`DisposalService` PLATFORM 雏形）扩展为项目级投递缺口告警（对账指标越限 → 控制台注意事项 + 通知）。
- runbook：备份通道开启/关闭/验证步骤、与桥接修复的交接顺序、演练脚本。

### Phase 3 — 转正决策点（用户裁决）

凭 Phase 1/2 双跑数据三选一：

- (a) **维持「桥主备从」**：桥健康运行，备份通道作为保险常开；
- (b) **备份通道转正**：试点项目退役 OC 映射依赖（桥只留存量老项目），逐项目推广；
- (c) **TB 旁路（完全体）**：NS→DeviceHub→RocketMQ 直达，TB 退居治理面/运维视图——"TB 作为遥测备选通道"至此完成使命。

## 6. 风险清单

| 风险 | 应对 |
|---|---|
| 同 clientId 互踢现网桥接 | 备份通道独立 clientId + 专用凭据；Phase 0 ①前置验证 |
| ts 对齐失败 → 双跑重复帧 | Phase 0 ③硬门槛，不达标不 Go |
| TB 规则链对 REST 写入不触发解码 → 备份期业务字段降级 | 明示为备份语义（保数不保解码）；业务侧按契约兜底帧语义消费（`telemetry-event-contract.md` 第 3/4 节已定义） |
| 备份通道自身故障成为新的静默丢弃点 | 所有丢弃计数 + 控制台可见；订阅断开告警 |
| TB 存储翻倍 | 当前遥测量级小，可接受；Phase 3 转正后可反向精简 |
| 与旧代际链路（body/data 设备）混淆 | 备份通道只对 registered_devices 落库，天然不碰旧代际设备 |
| NS broker 对长连/持久会话/QoS 的限制 | Phase 0 ①前置验证并记录降级路径 |

## 7. 与 219 事件的对照（价值锚点）

若 2026-09-28 本方案已运行：3588/3c31 均为 DeviceHub 注册设备 → 966 条消息全部经通配订阅写入 TB `dataHex` → 设备级对账立即见到兜底帧，故障发现时间从"数小时"缩到"分钟级"（投递缺口直接报警），且**一帧都不丢**、无需任何人执行任何修复动作。

## 8. Phase 0 spike 实施记录（2026-09-29）

> 执行环境：dev 服务器 172.17.10.206；探针工件在 `/tmp/backup-spike/`。OC broker 凭据每次从 TB 共享属性 `OC` 现取现用、用后即删，TB 密码只经容器环境变量传递，全程不落盘不回显。

| # | 验证项 | 结果 | 证据 |
|---|---|---|---|
| ① | 通配订阅长连 24h | **运行中**（09:46 CST 起）：clientId `dhbk_spike_*`（独立于桥 `gw_*`）、QoS1+cleanSession=false、订阅 `org/1/project/#`，全总线 ~26 msg/min、10+ 项目在流、预估 ~11MB/天。**离线补收 PASS**：停 240s 后重启，缺口期 110 条全部补收——broker 支持持久会话排队，「断桥零丢帧」前提成立 | 容器 `bks-spike-24h`（--restart unless-stopped）、`msgs.log`（`%U\|topic\|payload`） |
| ② | TB REST 直写 + 规则链 | **PASS**：`POST /api/plugins/telemetry/DEVICE/{id}/timeseries/SERVER_SCOPE`（**TB 3.8 路径带 `{scope}` 段，旧版 `/TIMESERIES` 路由已 500**）写 `dataHex` 200；**显式 ts 毫秒精确落库**（1970 类 bug 从根上排除）；**规则链不触发解码**（读回无 `result`）→ 备份语义=保数不保解码，与 §3 设计一致 | 写入 ts=1790646684763 精确回读；试点=已退役 3588（devicehub-app 日志 0 触达，零业务影响） |
| ③ | 双通道 ts 对齐（硬门槛 ≤2s） | **趋势 PASS（n=17）**：219 回测 1 帧 565ms + 通配日志 16 帧/12 设备；\|delta\| 中位 271.5ms、max 565ms、within-2s 17/17；TB ts 一致略晚于 NS 源 ts 200~600ms（桥的到达/处理时延口径）。备份路径写 `round(ts×1000)`，与桥路径天然同窗。**2h 自动采样持续累积，24h 满窗后终审** | `align3.log`（采样循环 pid 在 `sampler.pid`，`run_align3.sh` 每次现取 TB token） |
| ④ | NS 消息 ts 单位 | **定论：epoch 秒（浮点，含亚秒）**——bus219.log 47/47 + 24h 日志一致；归一化公式 `ts_ms = round(ts × 1000)`（Phase 1 单测锁定）；旧代际 1970 bug = 把秒当毫秒存的直接后果 | bus219.log、msgs.log |
| ⑤ | 桥接宿主机 | 未动（非阻塞）；指纹在案（双 1883 连接 + clientId `gw_pLoh7THJzthWrYDLk2mE_2`），下次接触运维时补 | 项目记忆 gateway-mapping-gap-r07 |

**顺带产出——DeviceHub 第一份「全总线视角」观测**：09:46–09:53 七分钟内总线 57 台唯一设备在流，其中 **44 台不在 TB**（未注册/历史设备）。备份通道落地后此计数常态化进控制台。

**已知边缘（记录在案，不阻塞）**：TB 对 REST 写入的字符串做数值强转（`"00"`→`0`）；真实 dataHex 均为 `aGt0…` 开头的 base64 不会触发；桥路径同行为，非备份通道新增风险。

**Go/No-Go 初判**：④ 口径已定、② 通过、③ 大概率达标——按方案推进 Phase 1 准备；**24h 满窗复核（① 存活 + ③ 终审）作为正式 Go 门槛**。



| 判定 | 出处 |
|---|---|
| 链路架构与收口原则 | `README.md:7-9`、`docs/integration-cutover.md:8-21,47` |
| 映射存储/不热重载/connector_reboot 坏/条目脆弱 | `docs/runbook-gateway-mapping.md:8-16,46,100-110`、`application.yml:39,42,43` |
| 966 条丢帧实锤、总线通配在流、宿主机排查、指纹 | 219 事件复盘（2026-09-28，项目记忆 gateway-mapping-gap-r07） |
| 旧代际直推证据与契约缺口 | 知识库 2026-09-16《新设备 TB 有遥测但开通 preflight 不过的归因》 |
| frameId 通道无关 / 2s 窗口 / 兜底帧 | `TbFrameNormalizer.java:38,89-95,130-138,147-151`、`docs/telemetry-event-contract.md:26,33-55` |
| PLATFORM 分诊雏形 | `DisposalService.java:100-132` |
| 巡检/一键修复端点 | `GatewayMappingController.java:30,40` |
| ChirpStack v3 TB 集成 per-device token 机制 | ChirpStack v3 官方文档 application-server/integrations/thingsboard |
| DeviceHub 无 MQTT 依赖 | `pom.xml:140`（仅 rocketmq） |
