

在代码库 /Volumes/DEV/00-products-dev/01-solutions/02-smart-livestock/smart-livestock-server 中调查真实设备遥测接入链路，回答以下问题（搜索广度：medium）： 背景：test 环境两台真实设备 9/12、9/14 各自有数据，9/15（今天）没有数据。设备1 = 瘤胃胶囊 RBC-001a0103ff000265（LoRaWAN 868/CN470 频段），设备2 = 动物追踪器 0095690a00008c91（915 频段，GPS TRACKER 类型）。 请查明： 1. 真实设备遥测有哪几条接入通道？（ThingsBoard TB 通道、中台 agentic platform 同步 job、NS 预置通道等）每条通道的入口类/定时任务类名和文件路径。特别是：瘤胃胶囊数据走哪条/哪几条通道？动物追踪器（TRACKER，GPS）走哪条通道？ 2. 每次遥测入库会写哪些表？device_telemetry_logs 是否每条遥测都写（作为原始入库记录）？temperature_logs / rumen_motility_logs / activity_logs / gps_logs 的写入条件（设备类型、payload 内容）？ 3. devices 表是否有 last_active_at / last_seen 之类的字段，在哪里被更新？设备离线判定/离线告警逻辑在哪？ 4. 中台同步 job 和 TB 通道各自的"同步游标"机制：游标存在哪、如何推进、什么情况下会卡住不推进（例如解析失败、时间 fallback now() 等）？ 5. gps_ingestion_tasks outbox 的调度器逻辑：PENDING/FAILED 状态、重试上限、什么错误会导致任务卡住。 6. 是否存在任何"数据今天突然没有"的代码层可能：例如 30 天数据清理、分区维护、同步窗口上限、轮询频率等定时任务配置。 输出：每个问题给出结论 + 关键类名/文件路径:行号。不要贴大段代码，只给结论和位置。

调查完成。以下按问题给出结论（路径均在 `/Volumes/DEV/00-products-dev/01-solutions/02-smart-livestock/smart-livestock-server/src/main/java/com/smartlivestock/` 下，简写为 `…`）。

## 1. 遥测接入通道

所有通道最终汇入统一入口 `TelemetryIngestionService.ingest()`（`…/iot/application/TelemetryIngestionService.java:76`），按 `TelemetrySource` 枚举区分（`…/iot/domain/model/TelemetrySource.java`）：

| 通道            | 入口/定时类                                                  | 调度                                                    | 开关                                                         |
| --------------- | ------------------------------------------------------------ | ------------------------------------------------------- | ------------------------------------------------------------ |
| 中台(blade)轮询 | `AgenticPlatformSyncDispatcher.dispatch()` `…/iot/application/AgenticPlatformSyncDispatcher.java:78`（fixedDelay 300s）→ `AgenticPlatformTelemetrySyncJob.syncDevice()` `…/iot/application/AgenticPlatformTelemetrySyncJob.java:59` → `ingest(AGENTIC_PLATFORM)` | 5 分钟/轮，线程池并发 5，上一轮未完成整轮跳过（:93-99） | `agentic-platform.sync.enabled`（application.yml:99 默认 false，test 用 .env 覆盖） |
| TB 通道         | `TbTelemetryChannel.poll()` `…/iot/application/TbTelemetryChannel.java:48`（fixedDelay 300s）→ `TbTelemetryFrameParser.extract()` → `ingest(THINGSBOARD)` | 5 分钟/轮，每设备每轮最多 50 页 × batch 200             | `smartlivestock.tb.enabled`（application.yml:112 默认 false） |
| NS 预置         | `TbDeviceProvisioningService.reconcile()` `…/iot/application/TbDeviceProvisioningService.java:49`（NS 列设备 → 建/校验 `tb_device_bindings`，profile：瘤胃胶囊-OC-配置-v2 / 牛羊追踪器-OC-配置-v2，:37-38）。**NS 不拉遥测**，只为 TB 通道准备 RESOLVED binding；controller 手动触发（`TbDeviceProvisioningController`） | 无定时，手动                                            | `smartlivestock.ns.enabled`                                  |
| HTTP 推送       | `TelemetryController` POST `/api/v1/farms/{farmId}/telemetry` `…/iot/interfaces/TelemetryController.java:38` → `ingest(HTTP)` | 无                                                      | -                                                            |
| datagen 合成    | `SynthesisRunner` `…/datagen/application/SynthesisRunner.java:23`（fixedRate 10s，`datagen.enabled` 默认 **true**）→ `SynthesisService:88` → `ingest(DATAGEN)` | 10 秒                                                   | datagen 设备                                                 |
| 手动导入        | `TelemetryImportService:174`（MANUAL_IMPORT，只写历史行，不碰设备快照） | 无                                                      | -                                                            |

两台设备的走向（代码层判定，与频段无关）：
- **瘤胃胶囊 RBC-001…（CAPSULE）**：走中台通道（`AgenticPlatformTelemetrySyncJob:181` + `AgenticPlatformReportData.toReadings():73` 用 `RumenPayloadDecoder` 解 hexData）和/或 TB 通道（result 里 decodeData.properties 含 gastricMotility/temperatureGroup，或 dataHex hex fallback 解码，`TbTelemetryFrameParser:151-157,161-169`）。是否双跑由 `smartlivestock.tb.blade-exclusion`（默认 false=双跑，靠 (device_id,report_time) 幂等去重）+ TB 健康度降级 `TbDeviceBinding.isTbChannelHealthy`（`…/iot/domain/model/TbDeviceBinding.java:46`）决定。
- **动物追踪器 0095690a…（TRACKER，supportsGps=true）**：同样中台/TB 二选一或双跑；decodeData 走 blade 的 latitude/longitude/stepNumber/加速度原始值键（`AgenticPlatformReportData:89-102` + `applyAccelerometerConversion:131`）。
- 遗留 RocketMQ worker `AgenticPlatformSyncWorker`（`…/iot/infrastructure/mq/AgenticPlatformSyncWorker.java:27`）仅当 `agentic-platform.sync.use-rocketmq=true` 才启用，现已改为 dispatcher 直连。

## 2. 每次遥测写哪些表

`ingest()` 一次事务内（`TelemetryIngestionService`）：
1. **devices 快照**（battery/rssi/snr/gateway/antiDisassembly + `lastOnlineAt=now()`，:237-256；MANUAL_IMPORT 跳过）。
2. **device_telemetry_logs：每条遥测必写一行**（`logDeviceTelemetry:273-308`），幂等唯一索引 `uq_dtl_device_report_time (device_id, report_time)`（`db/migration/V20260718120000`）。
3. **gps_ingestion_tasks（outbox）**：仅 `deviceType.supportsGps()`（TRACKER/EAR_TAG）且 readings 有合法 lat/lng（非 0,0、范围校验，`enqueueGps:310-347`）。CAPSULE 永不写 GPS。
4. AGENTIC_PLATFORM 源时推进 `devices.last_telemetry_synced_at = reportTime`（:142-145）。
5. 发布 `TelemetryReceivedEvent` → `SpringEventPublisher:54` → RocketMQ topic `telemetry-received` → **异步** `TelemetryEventConsumer`（`…/health/infrastructure/mq/TelemetryEventConsumer.java:21`）→ `HealthApplicationService.processTelemetry`（`…/health/application/service/HealthApplicationService.java:79`）：
   - **CAPSULE**：`temperature_logs`（有 `temperature` 或 `temperatures[]` 数组按 5 分钟/点回填，:90-103；幂等 existsByDeviceId+recordedAt+source :238）；`rumen_motility_logs`（有 `gastricMotility`，累积计数源用 counter 差分推频率 :105-127）；`activity_logs`（activityIndex/stepCount/distanceMeters 任一非空 :129-133）。
   - **TRACKER/EAR_TAG**：只写 `activity_logs`（:135-141），不写温度/蠕动。
   - 注意：这四张健康表依赖 RocketMQ 消费成功；MQ 断链时 device_telemetry_logs 有数、健康表无数。
6. **gps_logs** 由 outbox worker 异步写：`GpsIngestionTaskProcessor.processTask` → `GpsLogApplicationService.logGps`（`…/iot/application/GpsLogApplicationService.java:25`，upsert 幂等 `uq_gps_logs_device_recorded_at`，V20260720120000），成功后同事务删任务。

## 3. devices 在线字段与离线判定

- 字段：`last_online_at`（V3__create_iot_tables.sql:17）、`runtime_status`/`platform_device_id`/`last_telemetry_synced_at`（V20260709120000/V20260709150000）。**没有 last_active_at/last_seen**。
- 更新点：每次 ingest `updateDeviceRuntimeStatus` 里 `setLastOnlineAt(Instant.now())`（`TelemetryIngestionService.java:255`）；`Device.updateRuntimeStatus()`（`Device.java:104`）无任何调用方。
- `runtimeStatus` 仅在设备激活时从 blade `onlineStatus` 同步一次（`DeviceApplicationService.activateOnPlatform:302-318`）；展示层兜底：runtimeStatus 为 null 时 `last_online_at` 距今 <2h 视为 online（`DeviceDto.java:33-42`）。
- **离线告警逻辑不存在**：`AlertType` 只有 `DEVICE_TAMPER`/`DEVICE_LOW_BATTERY`（`…/ranch/domain/model/AlertType.java:12-13`），由 `TelemetryIngestionService.detectDeviceAlerts:349` 在中台/TB 源时创建；没有任何定时离线检测任务。

## 4. 同步游标机制

**TB 通道**（存 `tb_device_bindings.telemetry_cursor_ms` + `last_event_at`/`last_poll_at`/`consecutive_failures`）：
- 推进：`processBinding`（`TbTelemetryChannel.java:84-170`），startTs=cursor+1；首次无游标回看 `lookback-days`（默认 7 天，:97-99）；无失败时 cursor=max(成功帧 ts, 被丢弃的不可解码帧 ts)（:162-166）——不可解码帧故意跳过防止卡死；失败时只保留成功前缀（:153-159）。
- 卡住场景：页面/API/ingest 异常 → `consecutiveFailures++`，连续 ≥3 次或游标冻结 >15 分钟 → `isTbChannelHealthy=false` → 中台通道接管（若开了 blade-exclusion，`AgenticPlatformSyncDispatcher.refreshTbBoundDeviceIds:129`）；每轮 50 页上限截断，余量下轮续拉。

**中台通道**（游标存 `devices.last_telemetry_synced_at`）：
- 推进：每条记录在 `ingest()` 内推进为该条 reportTime（不是 now，`TelemetryIngestionService.java:136-145` 有注释说明曾用 now() 造成 8 小时缺口反复重拉）。
- 短路：先查 blade `lastActiveTime` ≤ cursor 直接整设备 skip（`AgenticPlatformTelemetrySyncJob.java:77-96`）。
- 卡住/跳数据场景：
  - **`parseReportTime` 解析失败 fallback `Instant.now()`**（`AgenticPlatformReportData.java:41-54`）→ cursor 一次性跳到当前时间，之前所有积压真实记录被 `>cursor` 过滤跳过（这就是“时间 fallback now() 卡住/丢数据”点）。
  - 分页上限 500 页、连续 pageSize 条旧记录 early exit（:151-162）；fetch 异常直接 throw，游标不动。
  - 设备非 ACTIVE → `ingest` 抛 ApiException → 该轮失败、游标不推进。

## 5. gps_ingestion_tasks outbox

- 表：V20260822200000__gps_ingestion_outbox.sql，unique (device_id, recorded_at)；enqueue 用 ON CONFLICT 重置回 PENDING/attempts=0（`SpringDataGpsIngestionTaskRepository.java:16-41`）。
- 调度：`GpsIngestionTaskScheduler` fixedDelay `gps.ingestion.poll-ms:500`，batch 100；只取 `status='PENDING' AND next_attempt_at<=now` 按 recorded_at 升序（:43-48）。
- 状态机（`GpsIngestionTask.markFailed:30-40`）：失败 → attempts++，<`max-attempts`(10) 回 PENDING + 30s 后重试；≥10 → **FAILED，永不重试、永不删除**（findReadyTaskIds 只查 PENDING）——即任何连续 10 次失败（如 DB 异常、约束冲突）都会让该 GPS 点永久卡死。
- 处理成功 = 写 gps_logs（upsert 幂等）+ 同事务删任务（`GpsIngestionTaskProcessor.processTask:27-50`）。

## 6. “数据今天突然没有”的代码层可能

按嫌疑排序：
1. **中台游标被 now() 污染**：reportTime 空/格式变化 → `parseReportTime` 返回 `Instant.now()`（`AgenticPlatformReportData.java:42,53`）→ cursor 跳到当下，之前记录全部被跳过，之后每条都盖“处理时刻”时间戳。
2. **dispatcher 背压整轮跳过**：上一轮未处理完 → 每 5 分钟整轮 skip（`AgenticPlatformSyncDispatcher.java:93-99`），若某设备同步 hang 住会无限期跳过。
3. **设备条件过滤**：`findActivePlatformDeviceIds` 只取 `status='ACTIVE' AND platform_device_id IS NOT NULL`（`SpringDataDeviceRepository.java:64`）；设备被置非 ACTIVE 或绑定丢失 → 静默不拉；TB 侧同样 skip 非 ACTIVE（`TbTelemetryChannel.java:90`）。
4. **TB consecutiveFailures / 全帧不可解码**（rule chain 改动导致 decodeStatus:false）→ 游标冻结 → 依赖 blade fallback 开关是否开启。
5. **健康表走 MQ**：`telemetry-received` 消费失败会 rethrow 重试；RocketMQ 停摆时仅健康表（temperature/rumen/activity）断流，device_telemetry_logs/gps_logs 正常。
6. 定时清理：`DataRetentionService.purgeOldData` cron `0 0 3 * * *`、30 天窗口（`…/datagen/application/DataRetentionService.java:39-50`），drop 整月分区 + 删 gps_logs/alerts；**该类无 @ConditionalOnProperty，datagen 关了也照跑**，但只删 30 天前数据，不会删今天。
7. 分区维护：`PartitionMaintenanceService` cron `0 10 2 * * *` + 启动时确保当月+2 月分区（`…/platform/application/PartitionMaintenanceService.java:36-48`）；若失败且当月分区缺失，遥测 insert 会报错（表现为通道失败+游标卡住，而非静默丢数）。
8. 窗口/频率上限：TB 300s 轮询、50 页×200 条/设备/轮，首次 lookback 仅 7 天；中台 300s、500 页上限——不影响“今天”，但影响追补速度。