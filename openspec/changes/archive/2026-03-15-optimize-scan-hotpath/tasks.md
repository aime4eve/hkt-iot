## 1. 数据结构优化

- [x] 1.1 将 `addressList: MutableList<String>` 替换为 `addressSet: HashSet<String>`，并引入 `deviceIndexMap: HashMap<String, Int>` 用于 address → mList index 映射。
- [x] 1.2 更新所有 `addressList` 引用点：`contains()` → `addressSet.contains()`，`add()` → `addressSet.add()`，`remove()` → `addressSet.remove()`，`clear()` → `addressSet.clear()`。
- [x] 1.3 更新 `deviceIndexMap` 的维护逻辑：在 `addDeviceList` 添加新设备时 `put(address, index)`，在 `filterDeviceList` 删除设备后重建映射，在 `scan(isNewSession=true)` 时 `clear()`。

## 2. RSSI 实时更新

- [x] 2.1 在 `addDeviceList()` 中，当 `addressSet.contains(address)` 为 true 时，通过 `deviceIndexMap[address]` 获取 index，更新 `mList[index].rssi` 为新值。
- [x] 2.2 在同一代码路径中，如果设备的 `name` 从 "N/A" 更新为非 null 值，同步更新 `mList[index].name`。
- [x] 2.3 标记需要 UI 刷新（设置 `pendingUiUpdate = true`）。

## 3. 过滤逻辑重构

- [x] 3.1 添加成员变量 `cachedRssiThreshold: Int` 用于缓存当前 RSSI 阈值，初始值从 SharedPreferences 读取。
- [x] 3.2 从 `addDeviceList()` 中移除 `filterDeviceList()` 调用和 `rssi = -getInt(RSSI, 100)` 的每次读取，改为使用 `cachedRssiThreshold`。
- [x] 3.3 在 `showScanFilterDialog` 的 SeekBar `onStopTrackingTouch` 中同步更新 `cachedRssiThreshold`，并调用 `filterDeviceList()` + 触发 UI 刷新。
- [x] 3.4 在 `showScanFilterDialog` 的 Switch `onCheckedChangeListener` 中，条件变更时调用 `filterDeviceList()` + 触发 UI 刷新。
- [x] 3.5 更新 `filterDeviceList()` 使其在删除设备后重建 `deviceIndexMap`。

## 4. UI 更新防抖

- [x] 4.1 添加成员变量 `pendingUiUpdate: Boolean = false` 和 `uiUpdateRunnable: Runnable`。
- [x] 4.2 实现 `scheduleUiUpdate()` 方法：若 `pendingUiUpdate` 为 false，设为 true 并 `bleHandler?.postDelayed(uiUpdateRunnable, 300)`。
- [x] 4.3 实现 `uiUpdateRunnable` 逻辑：执行 `adapterBluetoothList.updateItems(mList)`，更新可见性状态，重置 `pendingUiUpdate = false`。
- [x] 4.4 将 `addDeviceList()` 中所有直接调用 `adapterBluetoothList.updateItems()` 和可见性切换替换为 `scheduleUiUpdate()`。
- [x] 4.5 确保 `stopScan()` 中取消 pending 的 UI 更新 runnable 并执行最终一次刷新。

## 5. 验证

- [x] 5.1 运行 `./gradlew assembleDebug` 确认编译通过。
- [ ] 5.2 验证扫描能发现设备，列表正常显示。
- [ ] 5.3 验证同一设备的 RSSI 值在列表中持续更新。
- [ ] 5.4 验证修改过滤条件后列表正确过滤已有设备。
- [ ] 5.5 验证 3 轮扫描周期正常运行、手动停止正常工作。
