## ADDED Requirements

### Requirement: Spinner 下拉列表当前选中项蓝色高亮

系统 SHALL 提供一个自定义 `HighlightSpinnerAdapter`，当 Spinner 下拉列表展开时，当前选中项的文字 SHALL 以蓝色（colorPrimary #23ADE5）渲染，未选中项 SHALL 以默认黑色渲染。

#### Scenario: 下拉列表展开时高亮当前选中项
- **WHEN** 用户点击任意 Spinner 使下拉列表展开
- **THEN** 下拉列表中与 Spinner 当前 selectedItemPosition 匹配的列表项文字颜色 SHALL 为蓝色（#23ADE5），其余项文字颜色为黑色

#### Scenario: 用户切换选中项后高亮跟随
- **WHEN** 用户在下拉列表中选择了一个新选项，然后再次打开下拉列表
- **THEN** 新选中的项 SHALL 以蓝色字体显示，之前选中的项恢复为黑色字体

#### Scenario: Spinner 收起状态不受影响
- **WHEN** Spinner 处于收起（未展开）状态
- **THEN** Spinner 显示的选中项文字样式 SHALL 保持系统默认样式，不做颜色修改

### Requirement: 所有配置 Spinner 统一使用高亮适配器

应用中所有参数配置相关的 Spinner SHALL 统一使用 `HighlightSpinnerAdapter`，覆盖范围包括：
- SVC100 Config: `sp_vol_out`、`sp_valve_mode`、`sp_auto_power_on`、`sp_timezone`（4 个）
- DC200 Config: `sp_work_mode_dc200`（1 个）
- 实时任务 Config: `sp_valve_realtime_task`、`sp_open_state_realtime_task`（2 个）
- 定时任务 Config: `sp_task_number`、`sp_valve_timed`、`sp_open_state_timed`（3 个）

#### Scenario: SVC100 配置面板的 Spinner 使用高亮适配器
- **WHEN** 用户展开 SVC100 配置面板并点击任一 Spinner
- **THEN** 下拉列表中当前设备参数对应的选项 SHALL 以蓝色字体高亮显示

#### Scenario: DC200 配置面板的 Spinner 使用高亮适配器
- **WHEN** 用户展开 DC200 配置面板并点击工作模式 Spinner
- **THEN** 下拉列表中当前设备参数对应的选项 SHALL 以蓝色字体高亮显示

#### Scenario: 实时任务和定时任务面板的 Spinner 使用高亮适配器
- **WHEN** 用户展开实时任务或定时任务配置面板并点击任一 Spinner
- **THEN** 下拉列表中当前选中的选项 SHALL 以蓝色字体高亮显示

### Requirement: 自定义下拉列表项布局

系统 SHALL 提供自定义的 Spinner 下拉列表项布局文件 `spinner_dropdown_item_highlight.xml`，包含一个 `TextView`，用于在 `HighlightSpinnerAdapter` 中控制文字颜色和样式。

#### Scenario: 下拉列表项的基本样式
- **WHEN** Spinner 下拉列表展开
- **THEN** 每个列表项 SHALL 显示为单行文本，具有合适的 padding 和字号，与系统默认 `simple_spinner_dropdown_item` 的整体布局风格一致
