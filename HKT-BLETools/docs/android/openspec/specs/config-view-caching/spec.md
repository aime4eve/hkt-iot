## ADDED Requirements

### Requirement: Config 视图缓存机制
ExpandableListAdapter SHALL 为 Config 3（SVC100 参数）、Config 4（实时任务）、Config 5（定时任务）各维护一个缓存视图实例。`getGroupView` 在首次请求时创建并缓存视图，后续请求直接返回缓存实例。

#### Scenario: Config 3 视图缓存
- **WHEN** `getGroupView` 首次请求 Config 3（groupType=2, SVC100）的视图
- **THEN** 系统 SHALL 创建 `dialog_config_svc100` 视图、从 `mDeviceData` 填充当前参数值、缓存该视图实例并返回

#### Scenario: Config 3 视图复用
- **WHEN** `getGroupView` 再次请求 Config 3 的视图（缓存已存在）
- **THEN** 系统 SHALL 直接返回缓存视图，不重新创建，不覆盖 Spinner 选择和 EditText 输入

#### Scenario: Config 4 视图缓存与复用
- **WHEN** `getGroupView` 请求 Config 4（groupType=3, 实时任务）的视图
- **THEN** 系统 SHALL 在首次创建后缓存，后续直接返回缓存实例，保留用户填写的 valve、state、time、pulse 值

#### Scenario: Config 5 视图缓存与复用
- **WHEN** `getGroupView` 请求 Config 5（groupType=4, 定时任务）的视图
- **THEN** 系统 SHALL 在首次创建后缓存，后续直接返回缓存实例，保留用户填写的所有参数

### Requirement: Config 3 防抖时长正确显示
Config 3 视图中 `buffetingDurationEditText` SHALL 显示 `mDeviceData.buffetingDuration` 的值，而非 `mDeviceData.reportPeriod`。

#### Scenario: 初始加载时防抖时长显示
- **WHEN** Config 3 视图首次创建且 `mDeviceData.reportPeriod > 0`
- **THEN** `buffetingDurationEditText` SHALL 显示 `mDeviceData.buffetingDuration` 的值

### Requirement: Config 5 repeatTimed 位掩码正确计算
Config 5 定时任务的重复日选择 SHALL 将所有选中的星期正确计入 `mDeviceEvent.repeatTimed` 位掩码，包括第一个选中的项。

#### Scenario: 选中单个星期
- **WHEN** 用户在重复选择对话框中仅选中"周一"（index=0）
- **THEN** `mDeviceEvent.repeatTimed` SHALL 等于 `1`（即 `1 shl 0`）

#### Scenario: 选中多个星期
- **WHEN** 用户选中"周一"（index=0）和"周三"（index=2）
- **THEN** `mDeviceEvent.repeatTimed` SHALL 等于 `5`（即 `(1 shl 0) + (1 shl 2)`）

### Requirement: Config 5 重复选择首次点击即弹框
用户点击 `repeatTextView` 时 SHALL 立即弹出星期多选对话框，无需点击两次。

#### Scenario: 首次点击 repeatTextView
- **WHEN** 用户首次点击 repeatTextView
- **THEN** 系统 SHALL 立即显示包含 7 个星期选项的多选对话框

### Requirement: Config 5 时间验证逻辑修正
Config 5 的提交验证 SHALL 检查用户是否已选择开始时间和结束时间，而非依赖 `endTimeTimed - startTimeTimed <= 0` 的数值比较。

#### Scenario: 用户未选择时间
- **WHEN** 用户未点击选择开始时间或结束时间，直接点击 Config 5 按钮
- **THEN** 系统 SHALL 提示输入格式不正确

#### Scenario: 用户已选择有效时间
- **WHEN** 用户已通过 TimePicker 选择了开始时间和结束时间，且结束时间大于开始时间
- **THEN** 系统 SHALL 允许提交，不报格式错误
