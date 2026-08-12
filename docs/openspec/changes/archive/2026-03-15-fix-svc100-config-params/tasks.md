## 1. Config 3 防抖时长显示错误（无依赖，可最先执行）

- [x] 1.1 将 `getConfigSVC100View` 第 448 行 `buffetingDurationEditText.setText(mDeviceData.reportPeriod.toString())` 改为 `buffetingDurationEditText.setText(mDeviceData.buffetingDuration.toString())`

## 2. Config 3 视图缓存（依赖：任务 1 完成后执行）

- [x] 2.1 在 ExpandableListAdapter 中添加成员变量 `cachedConfigSVC100View: View? = null`
- [x] 2.2 修改 `getGroupView` 中 Config 3（groupType=2, SVC100）的分支：若 `cachedConfigSVC100View` 不为 null 则直接返回，跳过 inflate 和 tag 逻辑
- [x] 2.3 修改 `getConfigSVC100View`：首次创建时 inflate、从 `mDeviceData` 填充值、设置按钮监听器后存入 `cachedConfigSVC100View`；后续调用直接返回缓存视图

## 3. Config 4 视图缓存（依赖：任务 2 完成后执行，复用相同模式）

- [x] 3.1 在 ExpandableListAdapter 中添加成员变量 `cachedRealtimeTaskView: View? = null`
- [x] 3.2 修改 `getGroupView` 中 Config 4（groupType=3）的分支：若 `cachedRealtimeTaskView` 不为 null 则直接返回
- [x] 3.3 修改 `getRealtimeTask`：首次创建时 inflate 并存入 `cachedRealtimeTaskView`；后续调用直接返回缓存视图

## 4. Config 5 视图缓存（依赖：任务 3 完成后执行，复用相同模式）

- [x] 4.1 在 ExpandableListAdapter 中添加成员变量 `cachedTimedTaskView: View? = null`
- [x] 4.2 修改 `getGroupView` 中 Config 5（groupType=4）的分支：若 `cachedTimedTaskView` 不为 null 则直接返回
- [x] 4.3 修改 `getTimedTask`：首次创建时 inflate 并存入 `cachedTimedTaskView`；后续调用直接返回缓存视图

## 5. Config 5 repeatTimed 位掩码首项丢失（依赖：任务 4 完成后执行）

- [x] 5.1 在 `checkboxEdit`（Adapter 内的 TextView 版本）的确认回调中，将 `mDeviceEvent.repeatTimed += (1 shl i)` 从 else 分支移至 `if (selected[i])` 主体中，确保所有选中项（包括首项）都计入位掩码

## 6. Config 5 repeatTextView 嵌套 OnClickListener（依赖：任务 5 完成后执行）

- [x] 6.1 重构 `getTimedTask` 中 `repeatTextView` 的点击逻辑：将数据初始化（list、selected、str、id）移到 listener 外部，listener 内直接调用 `checkboxEdit`，去掉嵌套的 `setOnClickListener`

## 7. Config 5 时间验证逻辑（依赖：任务 6 完成后执行）

- [x] 7.1 在 `getTimedTask` 中添加 `startTimeSelected` 和 `endTimeSelected` 布尔标志，在 TimePicker 回调中置 true
- [x] 7.2 修改 configButton 点击验证：用 `startTimeSelected && endTimeSelected` 替代 `mDeviceEvent.endTimeTimed - mDeviceEvent.startTimeTimed <= 0`，未选时间时提示用户

## 8. 验证（依赖：任务 1-7 全部完成后执行）

- [ ] 8.1 Gradle 编译通过，无语法错误
