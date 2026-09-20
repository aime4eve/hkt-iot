## Why

当前所有 Spinner 下拉列表框使用系统默认样式渲染列表项，用户打开下拉列表后无法直观区分哪个选项是设备当前参数值（即从 BLE 设备读取回来的值）。这导致用户在修改参数前需要额外记忆当前值，降低了操作效率和界面体验。通过在下拉列表中用蓝色字体高亮当前选中项，可以给用户一个清晰的视觉提示。

## What Changes

- 创建自定义 Spinner 适配器 `HighlightSpinnerAdapter`，在下拉列表中检测当前选中位置，对匹配项使用蓝色字体渲染，其他项使用默认字体颜色。
- 创建自定义 Spinner dropdown item 布局文件，替代系统默认的 `simple_spinner_dropdown_item`。
- 将所有现有 Spinner（SVC100 的 4 个 Spinner、DC200 的 1 个 Spinner、实时任务的 2 个 Spinner、定时任务的 3 个 Spinner）的适配器替换为自定义适配器。
- 通过 XML `android:entries` 方式绑定的 Spinner 需改为代码中显式创建适配器。

## Capabilities

### New Capabilities
- `spinner-highlight-adapter`: 自定义 Spinner 适配器，支持在下拉列表中以蓝色字体高亮显示当前选中项，提升参数配置时的用户体验。

### Modified Capabilities

## Impact

- **代码文件**: `DeviceActivity.kt` 中的 `ExpandableListAdapter` 内部类，需为每个 Spinner 绑定自定义适配器。
- **新增文件**: 自定义 Adapter 类文件、自定义 Spinner dropdown item 布局 XML。
- **资源文件**: `colors.xml` 中可能新增高亮蓝色颜色值（或复用已有的 `colorPrimary` #23ADE5）。
- **影响范围**: 仅影响 UI 显示层，不影响 BLE 通信逻辑和参数读写逻辑。
