# HKT-DeviceHub

共享设备接入微服务：统一对接 ThingsBoard（TB）的**设备注册**与**上行遥测采集**，将规范化帧通过 RocketMQ 投递给下游业务项目（智慧畜牧 / 智慧停车）。二期预留**下行命令（TB server-side RPC）+ 设备调度**扩展点（本期仅占位，见 `docs/roadmap-commands.md`）。

## 架构（文字版）

```
LoRaWAN 设备 → NS → IoT Gateway (TB connector) → ThingsBoard
                                                      │
                 ┌────────────────────────────────────┴────────────────────┐
                 │                    HKT-DeviceHub                        │
                 │                                                        │
   REST 注册 ───▶│ DeviceProvisioningService ──▶ TB REST（建设备/查询/绑定）│
                 │        │                                               │
                 │        ▼                                               │
                 │  registered_devices (PostgreSQL, Flyway)               │
                 │        │ status=ACTIVE                                 │
                 │        ▼                                               │
                 │  TbWebSocketChannel（主通道，/api/ws 订阅，30s ping，    │
                 │   1s→60s 退避重连）                                     │
                 │  TbRestBackfillChannel（兜底，游标轮询回补断连缺口）     │
                 │        │                                               │
                 │        ▼                                               │
                 │  TbFrameNormalizer（result 权威 / dataHex fallback /    │
                 │   2s 同帧去重 / frameId 稳定哈希）                      │
                 │        │ TelemetryFrame                                │
                 │        ▼                                               │
                 │  TelemetryEventPublisher ──▶ RocketMQ                   │
                 └────────────────────────────│───────────────────────────┘
                                              ▼
                          topic DEVICEHUB_TELEMETRY_FRAME
                          tag=LIVESTOCK | PARKING, key=frameId
                                              ▼
                              下游业务项目（各自 consumer group，按 frameId 幂等）
```

配置与注册中心走 Nacos（data-id `devicehub.yaml`），本地 `application.yml` 为兜底。

## 组件

| 组件 | 位置 | 职责 |
|------|------|------|
| TbClient | `infrastructure/thingsboard` | TB REST：登录（token 缓存 50min、401 重登重放）、时序拉取、设备查询/创建；预留 `sendRpcOneWay/TwoWay`（本期仅供内部冒烟） |
| TbWebSocketChannel | `infrastructure/thingsboard` | WS 主通道，订阅 ACTIVE 设备的 result/dataHex/rssi/snr/downLinkGateway |
| TbRestBackfillChannel | `infrastructure/thingsboard` | REST 兜底回补；游标持久化到 `registered_devices.telemetry_cursor_ms`，成功前缀语义 |
| TbFrameNormalizer | `infrastructure/thingsboard` | TB timeseries → TelemetryFrame；2s 同帧去重；不可解码帧跳过并上报 |
| DeviceProvisioningService | `application/provisioning` | 注册编排（PENDING→ACTIVE/FAILED）、批量导入、对账；NS 对账为可选接口 `NsReconciliationPort`（默认 no-op） |
| TelemetryFrameDispatcher | `application` | 双通道统一出口：发布 MQ + 更新 lastEventAt + 指标 |
| TelemetryEventPublisher | `infrastructure/mq` | Spring Cloud Stream RocketMQ 生产者 |
| TelemetryChannelMetrics | `infrastructure/monitoring` | 通道状态、推送延迟、解析失败、重连、游标滞后（Prometheus） |

## 配置项

| 配置 | 环境变量 | 默认值 |
|------|----------|--------|
| server.port | `SERVER_PORT` | 8080 |
| spring.datasource.url | `DB_URL` | jdbc:postgresql://localhost:5432/devicehub |
| spring.datasource.username/password | `DB_USERNAME`/`DB_PASSWORD` | devicehub/devicehub（本地 compose） |
| spring.cloud.nacos.server-addr | `NACOS_SERVER_ADDR` | 172.22.3.16:8848 |
| spring.cloud.nacos.username/password/namespace | `NACOS_USERNAME`/`NACOS_PASSWORD`/`NACOS_NAMESPACE` | 空 |
| devicehub.tb.enabled | `TB_ENABLED` | false（本地默认关闭，dev compose 置 true） |
| devicehub.tb.base-url | `TB_BASE_URL` | http://172.22.3.105 |
| devicehub.tb.username/password | `TB_USERNAME`/`TB_PASSWORD` | tenant@hkt.com / 空（必须注入） |
| devicehub.tb.ws-ping-ms | `TB_WS_PING_MS` | 30000 |
| devicehub.tb.poll-interval-ms | `TB_POLL_INTERVAL_MS` | 300000 |
| devicehub.tb.lookback-days | `TB_LOOKBACK_DAYS` | 7 |
| devicehub.tb.batch-size | `TB_BATCH_SIZE` | 200 |
| devicehub.tb.last-frame-flush-ms | `TB_LAST_FRAME_FLUSH_MS` | 10000（WS 侧 last_frame_at 批量刷盘周期） |
| devicehub.tb.gateway-device-id | `TB_GATEWAY_DEVICE_ID` | 0f7da2b0-e91d-11ef-a8ee-99a8c68f9649（网关 OC 共享属性所在设备） |
| devicehub.ns.enabled | `NS_ENABLED` | false |
| devicehub.ns.base-url | `NS_BASE_URL` | http://172.17.201.15:8080 |
| devicehub.ns.username/password/org-id | `NS_USERNAME`/`NS_PASSWORD`/`NS_ORG_ID` | 空/空/1 |
| devicehub.profiles.<TYPE>.name | （application.yml） | CAPSULE=瘤胃胶囊-OC-配置-v2 / TRACKER=牛羊追踪器-OC-配置-v2 / GEOMAGNETIC=地磁-OC-配置-v2；profile id 运行时按名称解析，未硬编码 |
| devicehub.report-intervals.<TYPE> | （application.yml） | CAPSULE=14400 / TRACKER=60 / GEOMAGNETIC=3600 秒（自学习 TODO） |
| devicehub.topology.ns-host/ns-port | `NS_PROBE_HOST`/`NS_PROBE_PORT` | 172.17.201.15 / 1883 |
| devicehub.topology.probe-cache-ms | `TOPOLOGY_PROBE_CACHE_MS` | 30000 |
| devicehub.disposal.scan-interval-ms | `DISPOSAL_SCAN_INTERVAL_MS` | 300000 |
| spring.cloud.stream.rocketmq.binder.name-server | `ROCKETMQ_NAME_SERVER` | 172.17.10.206:9876 |
| telemetryFrame-out-0 destination | `DEVICEHUB_TELEMETRY_TOPIC` | DEVICEHUB_TELEMETRY_FRAME |

所有敏感值均走 `${ENV:default}` 占位，仓库内无任何真实密码。

## 本地启动

```bash
# 依赖连通性自检（Nacos / TB / RocketMQ / Postgres）
./scripts/check-env.sh

# 起依赖（postgres + rocketmq + dashboard，不启动 app 可注释掉 app 服务）
docker compose -f docker-compose.dev.yml up -d postgres rmqnamesrv rmqbroker rocketmq-dashboard

# 本地运行（TB 关闭，仅验证注册 API/迁移）
TB_ENABLED=false ./mvnw spring-boot:run
```

## dev 部署（172.17.10.206）

```bash
./scripts/deploy-dev.sh   # 打包 → scp → docker compose up -d → 连通性自检
```

端口规划：app 8080、namesrv 9876、broker 10911/10912、postgres 5432、rocketmq-dashboard 18080。

## REST API

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/v1/devices/register` | 单设备注册（devEui+project+externalRef+deviceType+capabilities） |
| POST | `/api/v1/devices/import` | 批量注册 |
| GET | `/api/v1/devices/reconcile?project=` | 对账：TB_MISSING / TB_ONLY / TB_IDENTITY_CONFLICT / CONSISTENT |
| GET | `/api/v1/devices` | 台账分页列表（project/status 筛选 + q 模糊：devEui/externalRef/deviceType） |
| GET | `/api/v1/devices/{id}` | 详情 + 最近 10 帧摘要（TB REST，null 伪影已过滤） |
| GET | `/api/v1/devices/preflight?devEui=&deviceType=` | 接入预检五项（R-01）：NS 归属 / 网关映射 / TB 唯一性 / profile 匹配 / 本地绑定，各 PASS/WARN/FAIL + 中文建议 + 依据字段 |
| GET | `/api/v1/devices/conflicts?project=` | TB 重名冲突清单（R-04），每台候选副本附创建时间/最近遥测/近似遥测条数/绑定方/建议保留 |
| DELETE | `/api/v1/devices/tb/{tbDeviceId}` | 删除 TB 空副本：服务端强制零遥测校验（含 null 伪影过滤）+ 本地未绑定校验，违反返回 422；成功与拒绝均写 audit_logs（operator 取请求头 X-Operator，默认 console） |
| GET | `/api/v1/channels/health` | WS 连接状态、各项目追赶滞后、无数据设备数、失败计数 |
| GET | `/api/v1/channels/topology` | 工作台拓扑聚合（R-10）：NS TCP 可达性、TB 认证、OC 映射项目清单、WS 订阅数 vs ACTIVE 一致性、MQ 24h 消息数（暂为 null+说明）、各项目追赶滞后/noDataDevices；探测缓存 30s |
| GET | `/api/v1/disposals?status=&project=` | 处置单列表 |
| GET | `/api/v1/disposals/{id}` | 处置单详情（含 timeline） |
| POST | `/api/v1/disposals/{id}/submit-approval?assignee=` | 分派并推送钉钉审批（mock）：PENDING_ASSIGN→APPROVING |
| POST | `/api/v1/disposals/{id}/approve-callback?approved=` | mock 钉钉回调：通过→PROCESSING，驳回→PENDING_ASSIGN |
| POST | `/api/v1/disposals/{id}/mark-handled` | PROCESSING→OBSERVING（观察截止=now+2×周期） |
| POST | `/api/v1/disposals/{id}/decommission` | 停用（body.reason 必填）：任意未关闭态→DECOMMISSIONED，设备转 DECOMMISSIONED |
| ANY | `/api/v1/devices/{id}/commands/**` | 二期占位，一律 501 |
| GET | `/actuator/health` `/actuator/prometheus` | 健康与指标 |
| GET | `/swagger-ui.html` | OpenAPI UI |

### 处置队列（console §3.4）

- **自动建单**：定时任务（默认 5min）扫描"注册超过 2×expectedReportIntervalSeconds 且 last_frame_at 为 null"的 ACTIVE 设备；同设备存在未关闭单时不重复建。沉默 >2×周期=NOTICE，>24h=CRITICAL。
- **自动分诊**：NS 查不到/离线/帧计数=0 → FIELD（现场类）；NS 有上行但 TB 无数据 → PLATFORM（平台类）；NS 不可用默认 FIELD 并记录证据。
- **观察期自动核销**：帧发布出口（TelemetryFrameDispatcher）挂钩子——OBSERVING 单的设备来帧即 RESOLVED 并记录 first_frame_at；观察超期由定时任务升级 CRITICAL + timeline（不流转状态）。
- **四级健康灯字段**（台账列表/详情返回 `health`）：`nsActivated`/`hasUplink` 待 NS 逐设备接口，当前恒为 null（文档标注）；`tbDecoded`=TB 侧有帧（含 dataHex fallback 未解码帧）；`ingested`=已发布 MQ。

### 健康端点字段语义（2026-09 调整）

`GET /api/v1/channels/health` 每项目聚合：

| 字段 | 语义 |
|------|------|
| `activeDevices` | status=ACTIVE 设备数 |
| `maxCursorLagMs` | **追赶滞后**（语义已变更）：项目内 `max(last_frame_at - telemetry_cursor_ms, 0)` 的最大值，即"TB 上有新帧但本地还没追平"。**不再是** `now - cursor`——沉默低频设备（游标停在最后一帧）报 0，不再虚报数天滞后。游标为 null 的设备按 0 计（首个回补周期建立游标）；项目内全部设备无数据时为 null。字段名保留以兼容已有看板。 |
| `noDataDevices` | 从未有任何帧的设备数（cursor 与 last_frame_at 均为 null） |
| `devicesWithFailures` / `totalConsecutiveFailures` | 失败计数（不变） |

`last_frame_at` 维护：REST 回补通道每周期对每设备做一次 `limit=1&orderBy=DESC` 廉价探测（忽略 TB 对无数据 key 返回的 `{ts:now, value:null}` 伪影条目）；WS 通道由 `LastFrameTracker` 内存累积每帧 ts、每 10s（`devicehub.tb.last-frame-flush-ms`）批量刷盘。该值只增不减。Prometheus gauge `devicehub.iot.cursor.lag.seconds` 同步改为同一追赶滞后语义。

## 事件契约

摘要：topic `DEVICEHUB_TELEMETRY_FRAME`，tag=项目（LIVESTOCK/PARKING），key=frameId，payload=`TelemetryFrame` JSON。完整字段表与乱序/重复语义见 `docs/telemetry-event-contract.md`。**消费方必须幂等、按 frameId 去重。**

## 二期扩展（下行命令 + 调度）

仅占位：`TbClient.sendRpcOneWay/TwoWay` 已封装但未暴露；`CommandController` 一律 501。设计要点（RPC 下行路径、Class A 窗口、命令状态机、`device_commands` 表、`application/command` 与 `application/scheduling` 包）见 `docs/roadmap-commands.md`。
