## Why

`addDeviceList()` 处于 BLE 扫描的热路径上，低延迟模式下每秒被调用数十到上百次。当前实现在每次回调中执行 O(n) 的列表查找、全量过滤和 DiffUtil 计算，随着发现设备增多性能急剧下降。同时，已发现设备的 RSSI 信号强度从不更新，导致列表中显示的信号值过时、RSSI 过滤对移动中的设备无效。

## What Changes

- **数据结构优化**：将 `addressList` 从 `ArrayList<String>` 替换为 `HashSet<String>`，使地址查重从 O(n) 降为 O(1)。
- **去除热路径全量过滤**：`filterDeviceList()` 不再在每次回调中调用，仅在过滤条件变更时触发一次。
- **RSSI 实时更新**：对已存在的设备，更新其 RSSI 值和显示名称，而非忽略后续广播。
- **UI 更新防抖**：引入防抖机制（~300ms），将高频回调攒批后统一刷新 RecyclerView，减少不必要的 DiffUtil 计算和 UI 重绘。

## Capabilities

### New Capabilities

- `scan-hotpath-optimization`: 定义扫描回调热路径的性能要求，包括数据结构选型、批量更新策略和 RSSI 实时更新行为。

### Modified Capabilities

- `bluetooth-scanning`: 更新扫描结果处理行为——已发现设备的 RSSI 需实时更新，过滤逻辑从每次回调执行改为条件变更时执行。

## Impact

- **受影响代码**：`MainActivity.kt`（`addDeviceList`、`filterDeviceList`、`scanCallback`、成员变量声明）。
- **用户体验**：设备列表响应更流畅，RSSI 值实时反映信号强度变化，大量设备场景下不再卡顿。
- **向后兼容**：无 API 变更，纯内部重构，用户行为不受影响。
