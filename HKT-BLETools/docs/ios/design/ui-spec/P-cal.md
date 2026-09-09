# 校准页（P_cal）— 规格卡（从冻结原型逐条提取）

> 事实源：`docs/ios/design/prototype/index.html`（已确认·冻结 2026-09-08）。
> 提取位置：`P_cal()`；`calBegin()`；CSS `.card/.badge/.progress/.center/.btn`；i18n `cal/calStart/calRun/calElapsed/cal90/calDC3/calNote/calDone/calFinish/calForUDS/calForDC/calEnvUDS/calPlaceUDS/calActUDS/cancel`，`calDCGuide` 7 项（ZH L324-331 / EN L426-433）。
> 原型变更时同一次提交更新本卡（§9 第 4 条）。

---

## 1. 页面结构（navbar.small「校准」，无首页按钮——showHome=false，与用户二级页裁决一致）

### idle（指引态）
```
.card（padding 13 14）
  ├─ 标题行：🧭 + 「磁力计校准/倾角校准」(16px/700，margin-bottom 10)
  ├─ 步骤列表：每项 水平(gap 12) —— 圆形序号 26×26(info 底白字 13px/700) + 文本(13px text2 行高1.6，
  │            有标题时「标题：」粗体前缀)；项间距 12
  ├─ 分隔线（上边框 line，margin-top 10 padding-top 10）：⚠︎ + calNote
.btn 全宽「开始校准」→ 弹确认对话框
```

### running（进行中）
```
.card 居中：badge(info)「校准进行中…（以设备上报为准）」
  + 「已用时 {s} 秒 · {时长提示}」(kv 13px text2)
  + 进度条（.progress：高 8 圆角 4 fill 底，info 填充，宽=min(100, elapsed/8×100)%）
.btn secondary 全宽「取消」→ 回详情页
```

### done（完成）
```
.center（min-height 380）：✓(46px ok 色) + 「✓ 校准完成」(17px/600) + 按钮「完成」(.btn 自适应宽 padding 12 40)
```

### 确认对话框（HKTDialog 296/r18）
- 标题「校准」；正文 = DC「请确认 7 项环境与操作要求后开始校准」/ UDS「将设备水平静止放置，然后开始校准」
- 按钮：secondary「取消」+ primary「开始校准」

## 2. 家族差异

| 家族 | 标题 | 步骤 | 进行中提示 |
|---|---|---|---|
| DC200Family（MPS/EPS） | 磁力计校准 / Magnetometer Calibration | calDCGuide 7 项（邻位车辆/固定铁磁结构/通电电缆/随身磁物/设备状态/触发并等待/验证与失败处理） | 最长约 3 分钟 |
| UDS100 | 倾角校准 / Tilt Calibration | 3 项（无序号标题，纯文本）：避免强磁与金属台面干扰…/设备水平静止放置/开始后保持设备完全静止… | 预计约 90 秒 |
| SVC100 | 无校准入口（详情页 CAL 瓷砖不渲染） | — | — |

## 3. iOS 状态与数据

- 入口：详情页 CAL 瓷砖（现有）→ push 校准页。
- 状态机：idle →（确认框）→ running（1s 计时，进度条按 8s 演示周期填充）→ done → 完成回详情。**真实校准命令（长时操作、以设备上报为准）随协议里程碑接入**，当前与原型演示一致为本地计时。
- 校准运行期间其他命令禁用（原型 opsDisabled）：iOS 为整页 push，详情页操作天然不可达。

## 4. 文案表

| key | ZH | EN |
|---|---|---|
| cal | 校准 | Calibration |
| calStart / cancel | 开始校准 / 取消 | Start Calibration / Cancel |
| calForDC | 请确认 7 项环境与操作要求后开始校准 | Confirm the seven environment and operation checks, then start |
| calForUDS | 将设备水平静止放置，然后开始校准 | Place the device level and still, then start |
| calRun | 校准进行中…（以设备上报为准） | Calibrating… (as reported by the device) |
| calElapsed(s) | 已用时 {s} 秒 | Elapsed {s}s |
| cal90 / calDC3 | 预计约 90 秒，请耐心等待 / 磁力计校准耗时较长（最长约 3 分钟），请耐心等待 | Expected ~90 seconds, please wait / Magnetometer calibration can take up to 3 minutes |
| calNote | 校准为长时操作，期间其他命令已禁用 | Long-running operation; other commands are disabled meanwhile |
| calDone / calFinish | ✓ 校准完成 / 完成 | ✓ Calibration complete / Done |
