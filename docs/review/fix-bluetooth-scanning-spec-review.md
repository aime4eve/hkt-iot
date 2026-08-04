# OpenSpec 任务评审报告

**评审目标**: `openspec/changes/fix-bluetooth-scanning/`
**评审日期**: 2026-03-15
**评审人**: Sisyphus

---

## 📋 总体评价

该 spec 任务存在**多个严重问题**，尤其是 spec.md 中的逻辑错误会导致实现方向完全错误。需要立即修正。

---

## 一、tasks.md 不足

| 序号 | 问题类型 | 问题描述 | 严重程度 | 建议 |
|------|----------|----------|----------|------|
| 1.1 | 路径不完整 | 任务仅说 "in `MainActivity.kt`"，未给出完整路径 | 中 | 添加完整路径，如 `app/src/main/java/com/example/bleTools/MainActivity.kt` |
| 1.2 | 基线缺失 | 未说明当前代码的 commit hash 或版本 | 中 | 添加 baseline commit 或代码片段 |
| 1.3 | 验证步骤模糊 | 任务 3.2 "Manual Verify bluetoothLeScanner is not null" 无具体操作步骤 | 中 | 改为：添加断点或日志输出验证 |
| 1.4 | 测试缺失 | 完全未提及单元测试或集成测试 | 高 | 应添加测试任务 |
| 1.5 | 验收标准缺失 | 任务无可量化的成功标准 | 中 | 应定义如"扫描到设备并显示"的具体场景 |

---

## 二、design.md 不足

| 序号 | 问题类型 | 问题描述 | 严重程度 | 建议 |
|------|----------|----------|----------|------|
| 2.1 | 无代码对比 | 决策部分无修改前后的代码示例 | 中 | 添加代码块对比 |
| 2.2 | 回滚计划缺失 | 迁移失败时无回滚方案 | 低 | 添加回滚步骤 |
| 2.3 | 影响分析不完整 | 提到 `BleCallback` 会受影响但未详细说明 | 中 | 补充说明或明确排除 |
| 2.4 | 边界情况缺失 | 未讨论权限拒绝、蓝牙关闭、扫描超时等 | 高 | 补充边界场景 |

---

## 三、proposal.md 不足

| 序号 | 问题类型 | 问题描述 | 严重程度 | 建议 |
|------|----------|----------|----------|------|
| 3.1 | 版本缺失 | 使用 `no.nordicsemi.android.support.v18:scanner` 但未指定版本号 | 中 | 添加版本号，如 `1.28.0` |
| 3.2 | Capability 空洞 | `bluetooth-scanning` 无详细 API 说明 | 中 | 补充接口定义 |
| 3.3 | Non-goal 不严谨 | 提到会修改 `BleCallback` 但列为 non-goal | 中 | 明确排除或包含 |

---

## 四、specs/bluetooth-scanning/spec.md 严重错误 ❌

### 4.1 逻辑错误（最高优先级）

**当前描述（错误）**:
```markdown
#### Scenario: Filter Null Names
- **WHEN** the "Filter Null Names" option is enabled (default)
- **AND** a device with a null name is discovered
- **THEN** the device is NOT added to the list
```

**问题分析**:
- 这段描述与要修复的 bug **完全矛盾**
- 当前 bug 是 `if (isScanNullNameDevice || bleDevice.device.name == null)` 使用 `||`，导致只要 `isScanNullNameDevice=true`，**所有设备**都被过滤
- 修复后应该是 `if (isScanNullNameDevice && bleDevice.device.name == null)`，即只有"启用过滤"**且**"设备名为空"时才过滤

**正确描述应为**:
```markdown
#### Scenario: Filter Null Names
- **WHEN** the "Filter Null Names" option is enabled (default: true)
- **AND** a device with a null/empty name is discovered
- **THEN** the device is NOT added to the list
- **BUT** if the option is disabled, devices with null names ARE shown
```

### 4.2 缺失的场景

| 序号 | 缺失场景 | 说明 |
|------|----------|------|
| 4.2.1 | 权限被拒绝 | 用户拒绝蓝牙权限时的错误提示和行为 |
| 4.2.2 | 蓝牙未开启 | 蓝牙关闭时的提示和引导开启 |
| 4.2.3 | 重复设备处理 | 同一设备多次发现时的更新逻辑 |
| 4.2.4 | 扫描超时 | 扫描是否有超时限制，超时后如何处理 |
| 4.2.5 | 扫描结果数量 | 是否有设备数量上限 |
| 4.2.6 | 错误处理 | 库初始化失败、扫描失败等错误场景 |
| 4.2.7 | Android 版本 | 未说明最低支持的 Android 版本 |
| 4.2.8 | 库初始化失败 | `BluetoothLeScannerCompat.getScanner()` 返回 null 时的处理 |

---

## 五、整体流程缺失

| 序号 | 问题 | 建议 |
|------|------|------|
| 5.1 | `.openspec.yaml` 信息不全 | 添加 owner、reviewer、deadline、status、priority 字段 |
| 5.2 | 无变更日志 | 添加 CHANGELOG.md 或在文件中记录历史 |
| 5.3 | 无关联评审 | 添加 PR 链接、code review 链接 |

---

## 六、改进优先级

### 🔴 高优先级（必须修复）
1. **修复 spec.md 中的 Filter Null Names 场景描述**（逻辑错误）
2. **添加测试用例任务**
3. **补充边界情况**（权限、蓝牙关闭、错误处理）

### 🟡 中优先级（建议改进）
4. 完善 tasks.md 中的文件路径和基线
5. 添加代码对比示例
6. 补充库版本号

### 🟢 低优先级（可选）
7. 完善 OpenSpec 元数据
8. 添加变更日志

---

## 七、建议的任务清单

如果需要重新修正，该 spec 任务应包含以下内容：

```
tasks.md:
├── 1. Library Migration
│   ├── 1.1 Remove native import (提供完整路径)
│   ├── 1.2 Change type to BluetoothLeScannerCompat
│   ├── 1.3 Initialize with getScanner()
│   ├── 1.4 Update scanCallback inheritance
│   ├── 1.5 Match callback signatures
│   └── 1.6 Add unit test for scanner initialization
├── 2. Logic Correction
│   ├── 2.1 Locate addDeviceList function
│   ├── 2.2 Fix || to &&
│   └── 2.3 Add integration test for device filtering
├── 3. Verification
│   ├── 3.1 Verify compilation
│   ├── 3.2 Manual test: scan shows devices
│   └── 3.3 Run existing tests
└── 4. Edge Cases (NEW)
    ├── 4.1 Handle permission denied
    ├── 4.2 Handle bluetooth disabled
    └── 4.3 Handle scanner init failure
```

---

**评审结论**: 需要返工，特别是 spec.md 中的逻辑错误必须立即修正，否则实现将偏离目标。
