# 阀门任务页（P_tasks）— 规格卡（从冻结原型逐条提取）

> 事实源：`docs/ios/design/prototype/index.html`（已确认·冻结 2026-09-08）。
> 提取位置：`P_tasks()` L1437-1480；`taskEditNew/taskEditExisting/nextTaskId/P_taskEdit/saveTask/delTask/delAllTasks/execRT` L1481-1560；`DAY_KEYS` L1433；固件契约注释 L1422-1431（0x03 载荷 6B=阀+开关+时长2B+脉冲2B，固件 3B 读取缺陷已记录 traceability §2.1-8b）。
> 原型变更时同一次提交更新本卡（§9 第 4 条）。

---

## 1. 列表页结构（仅 SVC100 会话可入）

```
navbar.small：back + 「阀门任务」+ navright linkbtn「＋ 新建任务」
body
  ├─ rtCard 实时任务卡（.card）：标题「实时任务 · 0x03」(700)
  │    ├─ fieldgrid 2 列：阀门（双阀/阀1/阀2 chips）+ 动作（开阀/关阀 chips）
  │    ├─ 水平 2 列 .field：持续时间(s) 输入+durHint / 脉冲数 输入
  │    ├─ [busy] err banner「⏱ 设备忙…（实时任务被静默忽略，无 ACK）」
  │    └─ .btn「立即执行」
  ├─ section「定时任务表 · 0x04 / 0x05」
  ├─ info banner「ℹ︎ 设备无 BLE 回读命令，以下为本机写入记录的镜像（固件仅支持 LoRa 平台侧回读 0x3D）」
  ├─ 任务卡 ×N：#id(b) + info badge「阀x · 开/关」+ 右 linkbtn「删除」(err 色)
  │             ⏰ sh:sm – eh:em · 脉冲 N(kv) + 星期 badge 行（选中日 info 底）
  ├─ [空] 居中卡「暂无定时任务」
  └─ [有任务] .btn danger「全部删除 (0xFF)」+ 居中 kv「删除执行中的任务将强制停止阀门动作」
```

## 2. 编辑页（新建/编辑共用）

```
navbar.small：back +「＋ 新建任务」
body（.card ×6）
  ├─ 任务槽位（1–16 方块格 38px；当前选中=info 白字；已占用=描边 outline；写入即覆盖 hint）
  ├─ 阀门 chips（双阀/阀1/阀2）
  ├─ 动作 chips（开阀/关阀）
  ├─ 脉冲数 输入
  ├─ 开始/结束 时间：时(00-23)+分(00-59) 两组下拉，中间 ↓
  ├─ 重复 chips：周一…周日（多选，选中=info 底）
  ├─ .btn「保存配置」（saveTask 校验：时分范围、结束>开始、脉冲 0–65535、至少选一天；失败 toast → iOS 行内错误横幅）
  └─ .btn secondary「取消」→ 回列表
```

## 3. 数据与命令（App 层本地镜像）

| 动作 | 命令 | 行为 |
|---|---|---|
| 立即执行 | 0x03（载荷 6B：阀+开关+时长2B+脉冲2B） | 校验 0–65535；busy 演示=err banner 2.6s；成功=ACK ✓ |
| 保存任务 | 0x04 | 前置校验通过 → 本地镜像 upsert（按槽位 id 覆盖）+ 日志 task #N ACK |
| 删除 | 0x05 | 移除镜像 + 日志 |
| 全部删除 | 0x05 (0xFF) | 清空镜像 + 日志；提示强制停止 |

- 真实 0x03/0x04/0x05 发送随协议里程碑接入；当前本地镜像+日志与原型演示一致。

## 4. 文案表

| key | ZH | EN |
|---|---|---|
| tasks / rt / sched | 阀门任务 / 实时任务 / 定时任务表（1–16） | Valve Tasks / Realtime Task / Schedule Table (1–16) |
| valve / v1 / v2 / vBoth | 阀门 / 阀 1 / 阀 2 / 双阀 | Valve / Valve 1 / Valve 2 / Both |
| act / openV / closeV | 动作 / 开阀 / 关阀 | Action / Open / Close |
| dur(Hint) / pulseN | 持续时间（0 = 不自动结束（须手动停止））/ 脉冲数 | Duration (0 = no auto-stop (stop manually)) / Pulse count |
| exec / busy | 立即执行 / 设备忙（本地定时任务执行中），实时任务被静默忽略（无 ACK） | Run Now / Device busy (schedule running); realtime task silently ignored (no ACK) |
| addTask / del / delAll | 新建任务 / 删除 / 全部删除 | New Task / Delete / Delete All |
| localMirror | 设备无 BLE 回读命令，以下为本机写入记录的镜像（固件仅支持 LoRa 平台侧回读 0x3D） | No BLE read-back; this list mirrors what was written from this phone (firmware exposes 0x3D via LoRa only) |
| noTasks / forceStopNote | 暂无定时任务 / 删除执行中的任务将强制停止阀门动作 | No scheduled tasks / Deleting a running task force-stops the valve |
| days | 一 二 三 四 五 六 日（用户 2026-09-10 裁决：中文单字） | Mon Tue Wed Thu Fri Sat Sun |
| slot(Hint) | 任务槽位（1–16；写入覆盖该槽位现有任务） | Task slot (1–16; writing overwrites the task in that slot) |
| needDay / invalidTime | 至少选择一天 / 时间无效 | Pick at least one day / Invalid time |
| d1..d7 | 周一 周二 周三 周四 周五 周六 周日 | Mon Tue Wed Thu Fri Sat Sun |

## 5. iOS 裁定

1. 入口：SVC100 详情页设备操作区新增「阀门任务」瓷砖（原型入口在演示工具栏，真机需要页面入口；DC/UDS 不显示）。
2. 校验失败提示：原型用 toast；iOS 在编辑页顶部显示 err 横幅（等价物）。
3. 任务镜像为内存态（与原型一致），持久化随诊断里程碑一并考虑。
