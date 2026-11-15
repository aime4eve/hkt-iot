# HKTPackTool — 地磁差分/全量升级发布包生成工具

独立 PC 工具（无外部依赖，单文件 C# 源码 + exe）。

## 构建

```
build_HKTPackTool.bat      （调用系统自带 .NET 4.x csc，产出 HKTPackTool.exe）
```

## 使用

双击 `HKTPackTool.exe`：

- **差分包**：选底包(旧 .bin) + 目标(新 .bin)，版本自动从文件名 `V<hw>.<sw>`
  识别（可手改），点「生成发布包」。
- **全量包**：切到全量，选镜像，版本自动读镜像偏移 0x200 的 `FTKH` 内嵌记录
  （无记录时按文件名/手填）。

输出 `<镜像名>_V<hw>.<sw>_diff.bin` / `_full.bin`（24B `HKTU` 头 + 载荷），
直接发给 APP「升级」页选择即可，APP 剥头只传载荷。

命令行（脚本调用）：

```
HKTPackTool.exe --diff <旧.bin> <新.bin> <out> <hw> <sw> <底包hw> <底包sw>
HKTPackTool.exe --full <镜像.bin> <out> <hw> <sw>
HKTPackTool.exe --selftest [旧工具补丁.bin]     # 回归自检（CRC/LZO/bsdiff 全电池）
```

## 工具内置防线（生成前全部强制校验，不过不出包）

1. **镜像防线**（镜像三重防线，与 APP `UpgradePackage.kt` / BL 同源）：
   大小 8192..180224、栈顶在 0x2000xxxx、复位向量在 0x08004000-0x0802FFFF。
2. **差分自检**：生成后用与固件 `app_bspatch.c` 逐语义一致的回放器
   （48B 头校验 → 段层 LZO 解压 + cs 字节和 → ctrl 边界 → diff/extra 合成）
   重放整个补丁，结果必须与目标镜像**逐字节一致**。
3. **收包防线**：按 Android `UpgradePackage.adopt` 镜像逻辑复检最终文件
   （magic/头版本/类型/载荷 CRC/长度/尺寸上限）。
4. **每段 LZO 回环**：压缩段必须经解码器还原一致才允许 `compress=1`，
   否则该段落回 raw 存储（固件两种都支持）。

## 产物格式（与固件/APP 契约一致）

- HKTU 发布头 24B（小端）：`"HKTU"` + 头版本1 + 类型(0全量/1差分) +
  目标hw/sw + 底包sw(0xFFFF=n/a) + 底包CRC32 + 载荷CRC32 + 载荷长度。
- 差分载荷 = 48B `DIFF` 头（patch_size/patch_crc/level=4096/pid=0x0007/
  cid=0xFFFF/新旧版本/新旧大小与CRC）+ 段层（`{u32 len, u8 compress, u8 cs}` ×N，
  4096B 分块 miniLZO 压缩）。
- LZO 为 `Compents/Upgrade/minilzo/minilzo.c` 的 1x-1 编码器/安全解码器
  机械移植（C#5），并以旧工具真补丁（1158B，真 miniLZO 输出）做过解码交叉验证。

## 设备侧核对项（工具无法代验）

- 底包 .bin 必须与设备正在运行的镜像**逐字节一致**（设备按 `old_fw_size`
  对运行镜像算 CRC32 比对 `old_fw_crc`）。
- 底包版本必须等于设备当前运行版本，目标版本必须等于 F0 申报版本。
