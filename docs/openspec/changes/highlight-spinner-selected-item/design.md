## Context

BLETools 应用中有 10 个 Spinner 分布在 4 个配置面板（SVC100 Config、DC200 Config、实时任务、定时任务）中。当前所有 Spinner 使用 Android 系统默认的 `simple_spinner_item` 和 `simple_spinner_dropdown_item` 布局，或通过 XML `android:entries` 属性自动绑定，下拉列表中所有选项的文字颜色相同，无法区分哪个是当前选中值。

现有 Spinner 绑定方式：
- **DC200**: 在代码中通过 `ArrayAdapter<String>` 显式创建适配器
- **SVC100/实时任务/定时任务**: 通过 XML `android:entries` 属性绑定 `arrays.xml` 中的字符串数组，由系统自动创建适配器

## Goals / Non-Goals

**Goals:**
- 在 Spinner 下拉列表展开时，当前选中项以蓝色字体显示，其他项保持默认颜色
- 所有 10 个 Spinner 统一使用该高亮效果
- 复用项目已有的蓝色配色 `colorPrimary`(#23ADE5)

**Non-Goals:**
- 不改变 Spinner 的收起状态（选中项显示）样式
- 不修改 Spinner 的边框、背景等其他视觉样式
- 不改变 BLE 参数读写逻辑

## Decisions

### Decision 1: 自定义 ArrayAdapter 而非自定义 View

**选择**: 创建一个继承 `ArrayAdapter<String>` 的 `HighlightSpinnerAdapter`，重写 `getDropDownView()` 方法。

**替代方案**: 
- 使用 `SpinnerAdapter` 包装器（Decorator 模式）—— 增加不必要的复杂度
- 在 `onItemSelectedListener` 中动态修改 View 颜色 —— 时机不可靠，且无法在下拉列表打开前设置

**理由**: `ArrayAdapter` 是项目已在使用的方式（DC200），重写 `getDropDownView()` 是 Android 官方推荐的自定义下拉项样式的方法，代码量最小且易维护。

### Decision 2: 颜色复用 colorPrimary

**选择**: 高亮蓝色使用项目已定义的 `colorPrimary`(#23ADE5)，默认颜色使用 `android.R.color.black`。

**理由**: 与应用的主色调保持一致，无需新增颜色资源。

### Decision 3: 创建自定义 dropdown item 布局

**选择**: 新建 `spinner_dropdown_item_highlight.xml` 布局文件，作为下拉列表项的模板，在 Adapter 的 `getDropDownView()` 中根据选中状态动态设置文字颜色。

**理由**: 使用自定义布局可以控制 padding、字号等细节，确保在不同设备上的显示一致性。

### Decision 4: 统一替换所有 Spinner 的 Adapter

**选择**: 移除 XML 中 `android:entries` 属性，统一在代码中为每个 Spinner 创建 `HighlightSpinnerAdapter` 实例。

**理由**: XML `android:entries` 自动创建的适配器无法自定义下拉项视图，必须改为代码创建适配器才能实现高亮效果。

## Risks / Trade-offs

- **[Risk] XML entries 移除后遗漏 Spinner** → 通过任务清单逐一核对所有 10 个 Spinner，确保无遗漏。
- **[Risk] Adapter 中 selectedItemPosition 获取时机** → `getDropDownView()` 在下拉展开时被调用，此时 `Spinner.selectedItemPosition` 已经是当前值，时机可靠。
- **[Trade-off] 代码量略有增加** → 每个 Spinner 需 3-4 行代码创建适配器，但换来了统一的视觉体验，可接受。
