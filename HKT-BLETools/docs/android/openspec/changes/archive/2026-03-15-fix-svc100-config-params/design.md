## Context

SVC100 电磁阀控制器在 `DeviceActivity.kt` 中通过 `ExpandableListAdapter` 展示 Config 3（设备参数）、Config 4（实时任务）、Config 5（定时任务）的配置界面。

当前架构：
- `counterRunnable` 每 1 秒执行一次，调用 `adapter.updateChildren()` → `notifyDataSetChanged()`
- `ExpandableListAdapter` 未覆写 `getGroupTypeCount()`，7 种不同布局的 Group 共用 1 个视图回收池
- `notifyDataSetChanged()` 后视图可能被回收并重新 inflate，导致用户输入（EditText/Spinner/TimePicker）全部丢失
- 现有 `isKeyboardVisible` 保护仅在键盘弹出时生效（EditText 聚焦），对 Spinner 操作无保护

涉及文件：`app/src/main/java/com/hkt/ble/bletools/DeviceActivity.kt`

## Goals / Non-Goals

**Goals:**
- Config 3/4/5 的用户输入在周期性刷新中稳定保持，不被覆盖或丢失
- Config 3 参数（voltage output、interface function、buffeting duration、auto power on、timezone、report period）可正常显示当前值并允许修改后提交
- Config 4 参数（valve、state、time、pulse）可正常填写并提交，不再报 "input format is incorrect"
- Config 5 参数（task number、valve、state、pulse、time、repeat）可正常填写并提交，不再报 "input format is incorrect"
- 修复所有已知代码逻辑 Bug

**Non-Goals:**
- 不重构整个 ExpandableListAdapter 架构
- 不修改 BLE 通信协议
- 不添加 Config 4/5 从设备读取当前配置的功能（设备协议未定义此能力）
- 不修改布局 XML 文件

## Decisions

### 决策 1: 采用视图缓存方案而非标志位方案

**选择**: 在 Adapter 中用成员变量缓存 Config 3/4/5 的视图实例，`getGroupView` 直接返回缓存视图。

**替代方案**: 添加 `isConfig3Editing` / `isConfig4Editing` / `isConfig5Editing` 标志位，在用户编辑时阻止刷新覆盖。

**理由**: 
- 视图缓存从根源消除视图重建问题，无需管理多个标志位的开/关时机
- Spinner 操作没有明确的"开始编辑"和"结束编辑"时机，标志位方案难以准确控制
- 缓存方案代码更简洁，每种 Config 视图固定一个实例，天然保留所有 UI 状态

### 决策 2: Config 3 只在视图首次创建时从 mDeviceData 填充

**选择**: `getConfigSVC100View` 中的 `setSelection`/`setText` 调用只在视图首次创建（缓存为 null）时执行，后续返回缓存视图时不再覆盖。

**理由**: 首次加载时显示设备当前参数值是合理的；之后用户编辑的值应该保留直到提交。

### 决策 3: Config 5 时间验证改为检查用户是否已选择时间

**选择**: 在 `mDeviceEvent` 中用标志或直接检查 EditText 内容是否为空来判断用户是否已选择开始/结束时间，而非依赖 `endTimeTimed - startTimeTimed <= 0`。

**理由**: 原逻辑在 startTime 和 endTime 默认值均为 0 时永远报错；且合法场景下起止时间可能相等（如 00:00 - 00:00 的全天任务），需要更精确的"用户是否已选"判断。

## Risks / Trade-offs

- **[Config 3 不自动同步设备参数更新]** → 视图缓存后，如果设备端参数被外部修改（如通过其他 App），Config 3 界面不会自动反映新值。缓解：首次加载时已读取最新值；用户可收起/展开 Config 组来刷新。这对配置场景影响极小。
- **[视图缓存增加内存占用]** → 3 个额外 View 引用，内存影响可忽略不计。
- **[修改 repeatTimed 位运算可能影响已有使用]** → 修复后 repeatTimed 的值会比修复前多包含首个选中项的位，但这才是协议要求的正确值。
