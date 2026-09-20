## 1. 资源文件准备

- [x] 1.1 创建自定义 Spinner 下拉列表项布局文件 `res/layout/spinner_dropdown_item_highlight.xml`，包含一个 `TextView`，设置合适的 padding 和字号，与系统 `simple_spinner_dropdown_item` 风格一致
- [x] 1.2 确认 `colors.xml` 中 `colorPrimary`(#23ADE5) 可复用为高亮蓝色，无需新增颜色资源

## 2. 自定义适配器

- [x] 2.1 在 `com.hkt.ble.bletools` 包下创建 `HighlightSpinnerAdapter.kt` 类，继承 `ArrayAdapter<String>`，持有对宿主 Spinner 的引用
- [x] 2.2 重写 `getDropDownView()` 方法：加载 `spinner_dropdown_item_highlight` 布局，比较当前 position 与 Spinner 的 `selectedItemPosition`，匹配则设置文字颜色为 `colorPrimary`，否则为黑色

## 3. 替换 SVC100 配置面板的 Spinner 适配器

- [x] 3.1 移除 `dialog_config_svc100.xml` 中 `sp_vol_out`、`sp_valve_mode`、`sp_auto_power_on`、`sp_timezone` 四个 Spinner 的 `android:entries` 属性
- [x] 3.2 在 `getConfigSVC100View()` 方法中，为上述 4 个 Spinner 分别创建 `HighlightSpinnerAdapter` 实例并绑定，数据源从对应的 `arrays.xml` 字符串数组中读取

## 4. 替换 DC200 配置面板的 Spinner 适配器

- [x] 4.1 在 `getConfigDC200View()` 方法中，将现有的 `ArrayAdapter<String>` 替换为 `HighlightSpinnerAdapter`，保持 items 列表不变

## 5. 替换实时任务配置面板的 Spinner 适配器

- [x] 5.1 移除 `dialog_config_realtime_task.xml` 中 `sp_valve_realtime_task`、`sp_open_state_realtime_task` 两个 Spinner 的 `android:entries` 属性
- [x] 5.2 在 `getRealtimeTask()` 方法中，为上述 2 个 Spinner 创建 `HighlightSpinnerAdapter` 实例并绑定

## 6. 替换定时任务配置面板的 Spinner 适配器

- [x] 6.1 移除 `dialog_config_timed_task.xml` 中 `sp_task_number`、`sp_valve_timed`、`sp_open_state_timed` 三个 Spinner 的 `android:entries` 属性
- [x] 6.2 在 `getTimedTask()` 方法中，为上述 3 个 Spinner 创建 `HighlightSpinnerAdapter` 实例并绑定

## 7. 验证

- [x] 7.1 编译项目确保无语法错误
- [ ] 7.2 在模拟器或真机上验证所有 10 个 Spinner 的下拉列表中，当前选中项均以蓝色字体高亮显示，切换选项后高亮正确跟随
