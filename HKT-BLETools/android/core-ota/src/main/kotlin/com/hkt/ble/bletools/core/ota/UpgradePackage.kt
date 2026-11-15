package com.hkt.ble.bletools.core.ota

import com.hkt.ble.bletools.core.protocol.CRC32
import com.hkt.ble.bletools.core.protocol.UpgradePkgType

/**
 * 升级发布包文件格式与收包防线（2026-11 差分/全量新升级通道的文件侧契约）。与帧层
 * [com.hkt.ble.bletools.core.protocol.UpgradeFrame] 分工：帧层约束"线上怎么传"，这里
 * 约束"发给用户的是什么文件、选文件时怎么校验"。
 *
 * 两种发布形态：
 * - **带包头的发布包**（24B 头 + 载荷）：差分包**必须**为此形态（只能由 PC 差分工具
 *   产生，无头差分一律拒绝——无头即来路不明）；全量也可用（发布脚本包一步），头内
 *   自带目标版本/包类型/底包信息/载荷 CRC。APP 剥头后只传输载荷，设备侧零改动
 *   （设备收到的仍是原始二进制，0xF0 申报的文件 CRC32 = 载荷 CRC32）。
 * - **裸全量镜像**：与烧录器烧写的同一份 .bin（单一产物，免再打包），目标版本取镜像
 *   内嵌版本记录（偏移 [FW_INFO_OFFSET]）；旧固件无记录时版本未知，仍允许升级
 *   （全量是救砖通道，设备侧才是权威），UI 显示"未知、以升级后读回为准"。
 *
 * 包头（24B 定长；多字节字段**小端**，与固件侧 C 结构体内存序一致，PC 打包工具同此布局）：
 * ```
 * 偏移  长度  字段
 * 0x00   4    magic "HKTU"
 * 0x04   1    头版本 = 1
 * 0x05   1    包类型（UpgradePkgType，与 0xF0 包类型字段同源）
 * 0x06   2    目标硬件版本
 * 0x08   2    目标软件版本
 * 0x0A   2    差分适用底包版本（全量 = 0xFFFF 不适用）
 * 0x0C   4    差分适用底包镜像 CRC32（全量 = 0）
 * 0x10   4    载荷 CRC32（= 0xF0 申报的"更新文件 CRC32"）
 * 0x14   4    载荷字节数
 * ```
 *
 * 镜像内嵌版本记录（固件 `g_fw_info`，链接钉在 APP 基址+0x200、向量表之后；小端）：
 * `magic "FTKH"(4) + hw_ver(2) + sw_ver(2) + 保留(4)` 共 12B。**版本唯一真源**——
 * BLE 版本上报、差分工具抄写入包头、APP 升级成功判核（重连读回 == 目标）三方同源；
 * 包头版本与内嵌记录不一致视为打包事故，直接拒绝。
 *
 * 分区真值（2026-11 新分区表，固件仓核实）：内部 BOOT 16KB / APP 0x08004000 起，
 * 0x08030000+ 为用户参数区不可被升级覆盖 → APP 镜像上限 0x0802FFFF（176KB）；
 * 栈顶须在 RAM、复位向量须落在 APP 区内（镜像三重防线与旧 OTA 同源）。
 */
object UpgradePackage {

    /** 发布包头定长与标识。 */
    const val HEADER_SIZE = 24
    const val HEADER_VERSION = 1
    private val HEADER_MAGIC = byteArrayOf(0x48, 0x4B, 0x54, 0x55)   // "HKTU"

    /** 全量镜像内嵌版本记录：偏移/定长与标识。 */
    const val FW_INFO_OFFSET = 0x200
    const val FW_INFO_SIZE = 12
    private val FW_INFO_MAGIC = byteArrayOf(0x46, 0x54, 0x4B, 0x48)   // "FTKH"

    /** 全量镜像大小合法域：新分区表 APP 区 0x08004000–0x0802FFFF（176KB），下限沿用 8KB。 */
    const val FULL_MIN_BYTES = 8_192
    const val FULL_MAX_BYTES = 180_224

    /** 差分包大小上界：外部 Patch 区 256KB。 */
    const val DIFF_MAX_BYTES = 262_144

    private const val APP_BASE = 0x0800_4000L
    private const val APP_END_EXCLUSIVE = 0x0803_0000L   // 用户参数区起点（开区间）

    /** 镜像内嵌版本记录解析结果。 */
    data class FwInfo(val hwVersion: Int, val swVersion: Int)

    /** 收包结果：通过（[Ok]）/ 防线拒绝（[Err]）。 */
    sealed interface Adopt {
        /**
         * 通过全部防线。[payload] 为设备实际收到的字节（带包头的发布包已剥头），
         * [swVersion] 为 null 表示文件未携带目标版本（裸全量无内嵌记录）——UI 显示
         * "未知"，不得编造（文件名提示由 UI 层兜底）。[baseSwVersion] 仅差分包有值，
         * 用于升级前底包匹配提示。
         */
        data class Ok(
            val payload: ByteArray,
            val pkgType: Int,
            val hwVersion: Int,
            val swVersion: Int?,
            val baseSwVersion: Int?,
            val payloadCrc32: Long,
            val versionSource: VersionSource,
        ) : Adopt

        /** 防线拒绝；[detail] 供日志，[actualType] 为 TYPE_MISMATCH 时头内真实包类型。 */
        data class Err(
            val reason: Reason,
            val detail: String,
            val actualType: Int? = null,
        ) : Adopt

        /** 目标版本来源（优先级：包头 > 镜像内嵌记录 > 无）。 */
        enum class VersionSource { WRAPPER, FW_INFO, NONE }

        enum class Reason {
            EMPTY,               // 文件为空
            TYPE_PENDING,        // 无包头且用户未选类型——待选择后再校验（非错误）
            TYPE_MISMATCH,       // 包头类型与用户所选不符
            HEADER_CORRUPT,      // 有 magic 但头字段不合法（版本/长度/类型）
            PAYLOAD_CRC_MISMATCH,// 包头申报的载荷 CRC32 与实际不符
            IMAGE_SIZE,          // 全量镜像大小超界
            IMAGE_STRUCTURE,     // 全量镜像栈顶/复位向量不合法
            FW_INFO_MISMATCH,    // 包头版本与镜像内嵌版本记录不一致
            DIFF_SIZE,           // 差分包大小超界/为空
            DIFF_NEEDS_HEADER,   // 差分包缺 HKTU 包头（只能由升级工具产生）
        }
    }

    /**
     * 收包防线总入口。[selectedType] 为用户当前显式选择（null=未选；仅带头文件可由
     * 包头代答类型）。带包头的文件先过头的结构/类型/CRC 三重校验再走类型专属防线；
     * 无头文件按所选类型走裸镜像防线（差分无头直接拒绝）。调用方在用户改选类型后
     * 须重新调用（防线与类型绑定）。
     */
    fun adopt(file: ByteArray, selectedType: Int?, fallbackHwVersion: Int): Adopt {
        if (file.isEmpty()) return Adopt.Err(Adopt.Reason.EMPTY, "0 B")

        if (hasHeaderMagic(file)) {
            // ---- 带包头：结构 → 类型 → CRC → 类型专属防线 ----
            val header = parseHeader(file)
                ?: return Adopt.Err(Adopt.Reason.HEADER_CORRUPT, "头字段不合法（版本/长度/包类型），${file.size} B")
            if (selectedType != null && selectedType != header.pkgType) {
                return Adopt.Err(
                    Adopt.Reason.TYPE_MISMATCH,
                    "包头类型=${header.pkgType}，所选=$selectedType",
                    header.pkgType,
                )
            }
            val payload = file.copyOfRange(HEADER_SIZE, file.size)
            val payloadCrc = CRC32.ieee(payload)
            if (payloadCrc != header.payloadCrc32) {
                return Adopt.Err(Adopt.Reason.PAYLOAD_CRC_MISMATCH, "头内 CRC32 ${"%08X".format(header.payloadCrc32)} ≠ 实际 ${"%08X".format(payloadCrc)}")
            }
            if (header.pkgType == UpgradePkgType.FULL) {
                fullImageError(payload)?.let { return Adopt.Err(it, "包内镜像 ${payload.size} B") }
                // 头版本与镜像内嵌记录（若有）必须一致——内嵌记录是唯一真源
                parseFwInfo(payload)?.let { fi ->
                    if (fi.hwVersion != header.hwVersion || fi.swVersion != header.swVersion) {
                        return Adopt.Err(
                            Adopt.Reason.FW_INFO_MISMATCH,
                            "头 v${header.hwVersion}.${header.swVersion} ≠ 内嵌 v${fi.hwVersion}.${fi.swVersion}",
                        )
                    }
                }
            } else {
                if (payload.isEmpty() || payload.size > DIFF_MAX_BYTES) {
                    return Adopt.Err(Adopt.Reason.DIFF_SIZE, "载荷 ${payload.size} B")
                }
            }
            return Adopt.Ok(
                payload = payload,
                pkgType = header.pkgType,
                hwVersion = header.hwVersion,
                swVersion = header.swVersion,
                baseSwVersion = header.baseSwVersion,
                payloadCrc32 = payloadCrc,
                versionSource = Adopt.VersionSource.WRAPPER,
            )
        }

        // ---- 无包头：全量可为裸镜像（烧录器同款 .bin），差分必须带头 ----
        if (selectedType == null) {
            return Adopt.Err(Adopt.Reason.TYPE_PENDING, "无包头且未选类型，待选择后校验")
        }
        if (selectedType == UpgradePkgType.DIFF) {
            val hint = if (looksLikeFullImage(file)) "；该文件结构疑似全量镜像，请核对包类型" else ""
            return Adopt.Err(Adopt.Reason.DIFF_NEEDS_HEADER, "缺 HKTU 发布包头$hint")
        }
        fullImageError(file)?.let { return Adopt.Err(it, "裸镜像 ${file.size} B") }
        val fi = parseFwInfo(file)
        return Adopt.Ok(
            payload = file,
            pkgType = UpgradePkgType.FULL,
            hwVersion = fi?.hwVersion ?: fallbackHwVersion,
            swVersion = fi?.swVersion,
            baseSwVersion = null,
            payloadCrc32 = CRC32.ieee(file),
            versionSource = if (fi != null) Adopt.VersionSource.FW_INFO else Adopt.VersionSource.NONE,
        )
    }

    /** 读镜像内嵌版本记录；无记录（旧固件/偏移越界/magic 不符）返回 null。 */
    private fun parseFwInfo(image: ByteArray): FwInfo? {
        if (image.size < FW_INFO_OFFSET + FW_INFO_SIZE) return null
        if (!FW_INFO_MAGIC.contentEquals(image.copyOfRange(FW_INFO_OFFSET, FW_INFO_OFFSET + 4))) return null
        return FwInfo(
            hwVersion = u16le(image, FW_INFO_OFFSET + 4),
            swVersion = u16le(image, FW_INFO_OFFSET + 6),
        )
    }

    private fun hasHeaderMagic(file: ByteArray): Boolean =
        file.size >= 4 && HEADER_MAGIC.contentEquals(file.copyOfRange(0, 4))

    /** 包头字段解析；magic 之外任一字段不合法返回 null。 */
    private fun parseHeader(file: ByteArray): PackageHeader? {
        if (file.size < HEADER_SIZE) return null
        if (file[4].toInt() and 0xFF != HEADER_VERSION) return null
        val pkgType = file[5].toInt() and 0xFF
        if (pkgType != UpgradePkgType.FULL && pkgType != UpgradePkgType.DIFF) return null
        val payloadLen = u32le(file, 20).toInt()
        if (payloadLen != file.size - HEADER_SIZE) return null
        val baseRaw = u16le(file, 10)
        return PackageHeader(
            pkgType = pkgType,
            hwVersion = u16le(file, 6),
            swVersion = u16le(file, 8),
            baseSwVersion = if (baseRaw == 0xFFFF) null else baseRaw,
            baseFwCrc32 = u32le(file, 12),
            payloadCrc32 = u32le(file, 16),
            payloadLen = payloadLen,
        )
    }

    private data class PackageHeader(
        val pkgType: Int,
        val hwVersion: Int,
        val swVersion: Int,
        val baseSwVersion: Int?,   // 0xFFFF = 不适用（全量）
        val baseFwCrc32: Long,     // 差分适用底包镜像 CRC32；全量 0（当前仅透出，不做比对）
        val payloadCrc32: Long,
        val payloadLen: Int,
    )

    /** 全量镜像三重防线：大小 → 栈顶在 RAM → 复位向量在新 APP 区；通过返回 null。 */
    private fun fullImageError(image: ByteArray): Adopt.Reason? {
        if (image.size !in FULL_MIN_BYTES..FULL_MAX_BYTES) return Adopt.Reason.IMAGE_SIZE
        val stackTop = u32le(image, 0)
        if (stackTop and 0xFFFE_0000L != 0x2000_0000L) return Adopt.Reason.IMAGE_STRUCTURE
        val resetVector = u32le(image, 4) and 0xFFFF_FFFEL   // 去 Thumb 位（bit0 清零）
        if (resetVector < APP_BASE || resetVector >= APP_END_EXCLUSIVE) return Adopt.Reason.IMAGE_STRUCTURE
        return null
    }

    /** 类型未定时的启发式（仅栈顶指针）：给"选了差分但拿的是全量镜像"的提示文案用。 */
    private fun looksLikeFullImage(file: ByteArray): Boolean =
        file.size >= 8 && u32le(file, 0) and 0xFFFE_0000L == 0x2000_0000L

    private fun u16le(b: ByteArray, off: Int): Int =
        (b[off].toInt() and 0xFF) or ((b[off + 1].toInt() and 0xFF) shl 8)

    private fun u32le(b: ByteArray, off: Int): Long =
        (b[off].toLong() and 0xFF) or
            ((b[off + 1].toLong() and 0xFF) shl 8) or
            ((b[off + 2].toLong() and 0xFF) shl 16) or
            ((b[off + 3].toLong() and 0xFF) shl 24)
}
