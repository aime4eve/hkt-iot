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
| POST | `/api/v1/devices/register` | 单设备注册（devEui+project+externalRef+capabilities） |
| POST | `/api/v1/devices/import` | 批量注册 |
| GET | `/api/v1/devices/reconcile?project=` | 对账：TB_MISSING / TB_ONLY / TB_IDENTITY_CONFLICT / CONSISTENT |
| GET | `/api/v1/channels/health` | WS 连接状态、各项目追赶滞后、无数据设备数、失败计数 |
| ANY | `/api/v1/devices/{id}/commands/**` | 二期占位，一律 501 |
| GET | `/actuator/health` `/actuator/prometheus` | 健康与指标 |
| GET | `/swagger-ui.html` | OpenAPI UI |

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
