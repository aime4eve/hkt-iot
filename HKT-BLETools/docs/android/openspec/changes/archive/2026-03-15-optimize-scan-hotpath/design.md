## Context

`MainActivity.kt` 的 `addDeviceList()` 方法是 BLE 扫描的核心回调处理器。在 `SCAN_MODE_LOW_LATENCY` 模式下，该方法每秒被调用数十到上百次。当前实现存在三个性能问题：

1. `addressList` 是 `ArrayList`，`contains()` 为 O(n)
2. `filterDeviceList()` 在每次回调中遍历整个设备列表
3. 每次回调都触发 `DiffUtil.calculateDiff()` 重新计算 + UI 刷新
4. 已发现设备的后续广播被丢弃，RSSI 永远不更新

## Goals / Non-Goals

**Goals:**
- 将热路径中的地址查重从 O(n) 降为 O(1)
- 移除热路径中不必要的全量列表遍历
- 实现已发现设备的 RSSI 实时更新
- 通过防抖机制减少 UI 刷新频率

**Non-Goals:**
- 不改变扫描参数（scan mode、callback type、cycle 策略）
- 不重构 `RecyclerViewListAdapter` 或 `BleDevice` 数据模型
- 不引入 Kotlin Coroutines 或 Flow（保持 Handler 风格一致）
- 不做线程安全改造（Nordic Scanner 在主线程回调，当前无并发风险）

## Decisions

### 1. addressList 替换为 HashSet

**决策**：将 `addressList: MutableList<String>` 替换为 `addressSet: HashSet<String>`。

**理由**：`HashSet.contains()` 是 O(1)，而 `ArrayList.contains()` 是 O(n)。在 50 台设备场景下，每次回调从 50 次比较降为 1 次哈希查找。只需改声明和调用处，风险极低。

**备选方案**：使用 `LinkedHashSet` 保留插入顺序——但地址列表仅用于查重，不需要顺序，普通 `HashSet` 足够。

### 2. RSSI 更新策略

**决策**：当设备已存在于列表中时，更新其 `rssi` 值和 `name`（如果 name 从 null 变为非 null）。

**理由**：
- RSSI 反映实时信号强度，对用户判断设备距离至关重要
- 某些设备首次广播可能不包含名称，后续广播才会携带
- 更新操作是 O(1)（通过 HashMap 直接定位）

**实现**：引入 `deviceMap: HashMap<String, Int>` 存储 address → mList index 的映射。当地址已存在时，通过 index 直接更新对应 `BleDevice` 的字段，避免遍历。

**备选方案**：用 `mList.indexOfFirst { it.device.address == address }` 查找——但这仍是 O(n)，不满足热路径优化目标。

### 3. 过滤逻辑重构

**决策**：`filterDeviceList()` 仅在过滤条件变更时调用（RSSI 阈值变化、null-name 开关切换），不在 `addDeviceList()` 热路径中调用。新设备添加时直接检查当前过滤条件。

**理由**：过滤条件在扫描过程中极少变化（只有用户主动修改 filter dialog 时才会变），每次回调都全量过滤完全没有必要。

**实现**：
- `addDeviceList()` 中删除 `filterDeviceList()` 调用
- `addDeviceList()` 中 RSSI 阈值直接用缓存的成员变量（在 filter dialog 关闭时更新），不再每次读 SharedPreferences
- `showScanFilterDialog` 关闭时调用一次 `filterDeviceList()` + 刷新 UI

### 4. UI 更新防抖

**决策**：引入一个 `pendingUiUpdate` 标志和 300ms 延迟的 Handler runnable，将高频 `updateItems()` 调用合并为低频批量刷新。

**理由**：在低延迟模式下，10 秒扫描周期内可能触发数百次回调。DiffUtil 虽然高效，但每次回调都计算 diff 并 dispatch 更新仍然会造成 UI 线程压力。300ms 间隔意味着每秒最多 ~3 次 UI 刷新，对用户来说视觉上无感知差异。

**备选方案**：
- 使用 `ListAdapter`（自带异步 DiffUtil）——改动更大，需要重构整个 Adapter
- 使用 `notifyItemInserted` / `notifyItemChanged` 替代 DiffUtil——逻辑更复杂，需要精确跟踪变更位置

## Risks / Trade-offs

- **RSSI 更新增加 DiffUtil 差异量**：每个周期内设备的 RSSI 都在变化，导致 DiffUtil 认为更多 item 有变化。→ 通过防抖机制抵消，300ms 内的多次变化只触发一次 diff。
- **防抖导致首个设备显示延迟 300ms**：第一个设备发现后，最多需等 300ms 才显示。→ 对于用户体验来说 300ms 不可感知，可接受。
- **deviceMap 额外内存占用**：新增一个 HashMap。→ 50 台设备只占几 KB，可忽略。
- **RSSI 阈值缓存可能与 SharedPreferences 不同步**：如果用户在扫描过程中修改 RSSI 阈值。→ 在 filter dialog 关闭时同步更新缓存值并触发一次过滤。
