# 集成拓扑与灰度切换规则

> 2026-09-20 定稿。本文是"两个业务单体与 HKT-DeviceHub 连接关系"的权威说明。
> 变更本文需同步：两单体 deployment 文档、98-hkt-iot README。

## 终态架构（灰度切换完成后）

```
设备 → NS → TB 网关(OC connector) → ThingsBoard
                                        │  （仅 HKT-DeviceHub 直连 TB）
                        WS 实时 + REST 游标回补
                                        ▼
                                HKT-DeviceHub
                                        │  RocketMQ: DEVICEHUB_TELEMETRY_FRAME
                                        │  REST: 注册/对账/绑定解析
                  ┌─────────────────────┴─────────────────────┐
                  ▼                                           ▼
         smart-livestock                              smart-parking
```

**核心原则：ThingsBoard 的访问收口到 HKT-DeviceHub 一处**，业务单体不直连 TB。

## 各项目终态连接关系

### smart-livestock

| 通道 | 终态 | 说明 |
|---|---|---|
| DeviceHub MQ 消费（tag=LIVESTOCK） | ✅ 保留 | `DeviceHubChannelConsumer` → `TelemetryIngestionService.ingest(..., THINGSBOARD)` |
| DeviceHub REST（注册/对账） | ✅ 保留 | `TbDeviceProvisioningController` 在 `devicehub.enabled=true` 时走 Feign 直连 |
| 本地 TB 轮询（TbTelemetryChannel） | ❌ 关闭 | `SMARTLIVESTOCK_TB_ENABLED=false`，代码保留一个迭代周期作回切备份 |
| blade 通道（AGENTIC_PLATFORM_*） | 不动 | 与 TB 无关的独立链路 |

注意点：
- provision 的 DeviceHub 路径不依赖本地 NS/TB 开关（绕过 `requireAutoconfigEnabled()`）
- `preflight` 预检端点目前仍走本地编排（用 NS+TB 客户端），关闭本地开关后 preflight 不可用——待迁移到 DeviceHub（改进项）
- 本地 TB 关闭后 provision 返回 `firstTelemetryTrigger=TB_TRIGGER_SKIPPED_DISABLED` 属正常：DeviceHub 注册成功后 5 分钟内 REST 回补自然拉到数据

### smart-parking

| 通道 | 终态 | 说明 |
|---|---|---|
| DeviceHub MQ 消费（tag=PARKING） | ✅ 保留 | `DeviceHubChannelConsumer`，去重走 `TelemetryDedupApplicationService`（channel=devicehub） |
| DeviceHub REST（绑定解析） | ✅ 保留 | `DeviceHubBindingResolver` 处理 THINGSBOARD pending 绑定 |
| 本地 TB WS 主通道 | ❌ 关闭 | `SMARTPARK_TB_WS_ENABLED=false` |
| 本地 TB REST 回补 | ❌ 关闭 | 随 WS 通道一并停用 |
| blade 通道（MQ 桥 + 轮询降级 + 注册） | 不动 | 既有独立集成（NIX-121），不经 TB |

parking 终态 = **blade + DeviceHub 两条通道**，ThingsBoard 不再直连。

## 灰度期（当前状态）

双跑是刻意的：本地 TB 通道与 DeviceHub 通道同时开启，靠各项目幂等机制保证同帧零重复（livestock `(device_id, report_time)` 唯一键；parking canonical key 仲裁，devicehub 优先级最低不置换 canonical）。

比对标准（已全部实测通过）：
- 同帧零重复（parking 211 条 devicehub 观测 208 条 MATCHED_DUPLICATE、0 置换；livestock 509 行零重复组）
- 断连 5 分钟内回补补齐
- 新设备经 DeviceHub 注册后两项目正常收数

## 切换操作步骤（单项目）

1. 确认灰度比对达标（见上）
2. 远程 env 关闭本地通道开关（livestock `SMARTLIVESTOCK_TB_ENABLED=false`；parking `SMARTPARK_TB_WS_ENABLED=false`）
3. 重部署该 dev 栈（`./scripts/deploy.sh dev`）
4. 冒烟验证：DeviceHub consumer 在收数、DeviceHub `/api/v1/channels/health` 游标推进、告警/状态机无异常
5. 保留本地通道代码一个迭代周期，无回切发生后再删代码

## 回退预案

任一项目出问题：把对应开关改回 `true` 重启即可恢复本地 TB 直连（分钟级）。DeviceHub 侧游标持续推进，回切期间的数据在恢复 DeviceHub 后由回补追平，双向都不丢数据。
