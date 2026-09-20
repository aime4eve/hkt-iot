## Why

SVC100 电磁阀控制器的 Config 3/4/5 参数配置功能存在严重可用性缺陷：Config 3 的参数修改后 1 秒内被刷回原值；Config 4 和 Config 5 点击配置按钮时始终提示 "The input format is incorrect"。根因是 `ExpandableListAdapter` 的 `counterRunnable` 每秒调用 `notifyDataSetChanged()` 导致视图被回收重建，用户输入丢失。此外还存在多个代码逻辑 Bug 导致数据显示错误和交互异常。

## What Changes

- 为 Config 3/4/5 视图引入**视图缓存机制**，每种 Config 视图只创建一次并缓存到 Adapter 成员变量，避免周期性刷新导致视图重建和用户输入丢失
- 修正 Config 3 中 `buffetingDurationEditText` 错误显示为 `reportPeriod` 的 Bug，改为正确显示 `buffetingDuration`
- 修正 Config 5 中 `repeatTimed` 位运算首项丢失的 Bug（第一个选中的星期未计入位掩码）
- 修正 Config 5 中 `repeatTextView` 嵌套 OnClickListener 导致首次点击不弹出选择框的 Bug
- 修正 Config 5 中时间验证逻辑：当用户未选择开始/结束时间时给出明确提示，而非一律报 "input format is incorrect"

## Capabilities

### New Capabilities
- `config-view-caching`: Config 3/4/5 视图缓存机制，确保视图状态在周期性刷新中不被破坏

### Modified Capabilities

## Impact

- `app/src/main/java/com/hkt/ble/bletools/DeviceActivity.kt` — ExpandableListAdapter 类的视图创建和管理逻辑
- 不涉及 BLE 通信协议变更
- 不涉及布局 XML 变更
- 不涉及新增依赖
