package com.hkt.ble.bletools.core.ota

import com.hkt.ble.bletools.core.ota.UpgradePackage.Adopt
import com.hkt.ble.bletools.core.protocol.CRC32
import com.hkt.ble.bletools.core.protocol.UpgradePkgType
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNull
import kotlin.test.assertTrue

/// 升级发布包收包防线测试：包头三重校验（结构/类型/CRC）、裸全量镜像防线、
/// 内嵌版本记录提取、版本来源优先级。字节布局见 UpgradePackage 类注释（全小端）。
class UpgradePackageTests {

    // ---- 构造工具：按包头布局手写（与 PC 打包工具/固件侧约定的同一张表）----

    private fun wrap(pkgType: Int, hw: Int, sw: Int, payload: ByteArray, baseSw: Int = 0xFFFF): ByteArray {
        val out = ByteArray(UpgradePackage.HEADER_SIZE + payload.size)
        out[0] = 0x48; out[1] = 0x4B; out[2] = 0x54; out[3] = 0x55   // "HKTU"
        out[4] = UpgradePackage.HEADER_VERSION.toByte()
        out[5] = pkgType.toByte()
        putU16le(out, 6, hw)
        putU16le(out, 8, sw)
        putU16le(out, 10, baseSw)
        putU32le(out, 12, 0)                                // base_fw_crc32（差分用，此处占位）
        putU32le(out, 16, CRC32.ieee(payload))
        putU32le(out, 20, payload.size.toLong())
        payload.copyInto(out, UpgradePackage.HEADER_SIZE)
        return out
    }

    /** 最小合法全量镜像：栈顶在 RAM + 复位向量在新 APP 区（Thumb 位），可选内嵌版本记录。 */
    private fun fullImage(size: Int = 8_192, fwInfo: Pair<Int, Int>? = null): ByteArray {
        val img = ByteArray(size)
        putU32le(img, 0, 0x2000_8000L)                       // 栈顶：RAM 区
        putU32le(img, 4, 0x0800_6101L)                       // 复位向量：APP 区内（bit0=Thumb）
        if (fwInfo != null) {
            img[UpgradePackage.FW_INFO_OFFSET] = 0x46        // "FTKH"
            img[UpgradePackage.FW_INFO_OFFSET + 1] = 0x54
            img[UpgradePackage.FW_INFO_OFFSET + 2] = 0x4B
            img[UpgradePackage.FW_INFO_OFFSET + 3] = 0x48
            putU16le(img, UpgradePackage.FW_INFO_OFFSET + 4, fwInfo.first)
            putU16le(img, UpgradePackage.FW_INFO_OFFSET + 6, fwInfo.second)
        }
        return img
    }

    private fun putU16le(b: ByteArray, off: Int, v: Int) {
        b[off] = (v and 0xFF).toByte()
        b[off + 1] = ((v shr 8) and 0xFF).toByte()
    }

    private fun putU32le(b: ByteArray, off: Int, v: Long) {
        b[off] = (v and 0xFF).toByte()
        b[off + 1] = ((v shr 8) and 0xFF).toByte()
        b[off + 2] = ((v shr 16) and 0xFF).toByte()
        b[off + 3] = ((v shr 24) and 0xFF).toByte()
    }

    private fun okOf(r: Adopt): Adopt.Ok = r as Adopt.Ok

    // ---- 带包头的发布包 ----

    @Test
    fun testWrappedDiffAdoptsAndStripsHeader() {
        val patch = ByteArray(1_000) { (it % 251).toByte() }
        val r = UpgradePackage.adopt(wrap(UpgradePkgType.DIFF, 2, 31, patch, baseSw = 30), UpgradePkgType.DIFF, 2)
        val ok = okOf(r)
        assertTrue(ok.payload.contentEquals(patch))                       // 剥头：设备收到的是纯 patch
        assertEquals(UpgradePkgType.DIFF, ok.pkgType)
        assertEquals(2, ok.hwVersion)
        assertEquals(31, ok.swVersion)
        assertEquals(30, ok.baseSwVersion)                                // 底包版本透出（UI 匹配提示用）
        assertEquals(CRC32.ieee(patch), ok.payloadCrc32)
        assertEquals(Adopt.VersionSource.WRAPPER, ok.versionSource)
    }

    @Test
    fun testWrappedFileTypeResolvesWithoutSelection() {
        // 用户未选类型时包头代答（文件名后缀提示的优先级高于头，但不得与头冲突报错）
        val r = UpgradePackage.adopt(wrap(UpgradePkgType.FULL, 2, 32, fullImage()), null, 2)
        assertEquals(UpgradePkgType.FULL, okOf(r).pkgType)
    }

    @Test
    fun testTypeMismatchReportsActualType() {
        val r = UpgradePackage.adopt(
            wrap(UpgradePkgType.DIFF, 2, 31, ByteArray(64)),
            UpgradePkgType.FULL, 2,
        )
        val err = r as Adopt.Err
        assertEquals(Adopt.Reason.TYPE_MISMATCH, err.reason)
        assertEquals(UpgradePkgType.DIFF, err.actualType)                 // 提示文案用：告诉用户真实类型
    }

    @Test
    fun testWrappedPayloadCrcMismatch() {
        val file = wrap(UpgradePkgType.DIFF, 2, 31, ByteArray(64)).also { it[40] = 0x7F } // 篡改载荷
        assertEquals(Adopt.Reason.PAYLOAD_CRC_MISMATCH, (UpgradePackage.adopt(file, UpgradePkgType.DIFF, 2) as Adopt.Err).reason)
    }

    @Test
    fun testHeaderLengthFieldCorrupt() {
        val file = wrap(UpgradePkgType.DIFF, 2, 31, ByteArray(64)).also { putU32le(it, 20, 999) } // 长度字段不实
        assertEquals(Adopt.Reason.HEADER_CORRUPT, (UpgradePackage.adopt(file, UpgradePkgType.DIFF, 2) as Adopt.Err).reason)
    }

    @Test
    fun testWrappedFullCrossChecksEmbeddedFwInfo() {
        // 头版本 == 内嵌记录：通过
        val good = wrap(UpgradePkgType.FULL, 2, 32, fullImage(fwInfo = 2 to 32))
        assertEquals(32, okOf(UpgradePackage.adopt(good, UpgradePkgType.FULL, 2)).swVersion)
        // 头版本 ≠ 内嵌记录：打包事故，拒绝（内嵌记录是唯一真源）
        val bad = wrap(UpgradePkgType.FULL, 2, 33, fullImage(fwInfo = 2 to 32))
        assertEquals(Adopt.Reason.FW_INFO_MISMATCH, (UpgradePackage.adopt(bad, UpgradePkgType.FULL, 2) as Adopt.Err).reason)
    }

    @Test
    fun testWrappedFullRejectsBadInnerImage() {
        // 包头本身合法，但载荷镜像复位向量跑到 BOOT 区 → 结构防线照走
        val img = fullImage().also { putU32le(it, 4, 0x0800_0101L) }
        val file = wrap(UpgradePkgType.FULL, 2, 32, img)
        assertEquals(Adopt.Reason.IMAGE_STRUCTURE, (UpgradePackage.adopt(file, UpgradePkgType.FULL, 2) as Adopt.Err).reason)
    }

    // ---- 裸全量镜像 ----

    @Test
    fun testRawFullReadsEmbeddedFwInfo() {
        val r = UpgradePackage.adopt(fullImage(fwInfo = 2 to 35), UpgradePkgType.FULL, 1)
        val ok = okOf(r)
        assertEquals(2, ok.hwVersion)                                     // 内嵌记录优先于设备兜底值
        assertEquals(35, ok.swVersion)
        assertEquals(Adopt.VersionSource.FW_INFO, ok.versionSource)
    }

    @Test
    fun testRawFullWithoutFwInfoIsUnknownButAllowed() {
        // 旧固件裸 bin：救砖语义允许继续，版本未知（不编造 0——这正是本次修的显示 bug）
        val r = UpgradePackage.adopt(fullImage(), UpgradePkgType.FULL, 2)
        val ok = okOf(r)
        assertNull(ok.swVersion)
        assertEquals(2, ok.hwVersion)                                     // hw 兜底沿用设备当前值
        assertEquals(Adopt.VersionSource.NONE, ok.versionSource)
    }

    @Test
    fun testRawFullRejectsNonImage() {
        // 栈顶不在 RAM（如误选的 HEX 文本，'.'=0x2E）→ 结构防线
        val hex = ByteArray(UpgradePackage.FULL_MIN_BYTES) { '.'.toByte() }
        val r = UpgradePackage.adopt(hex, UpgradePkgType.FULL, 2)
        assertEquals(Adopt.Reason.IMAGE_STRUCTURE, (r as Adopt.Err).reason)
    }

    @Test
    fun testRawFullRejectsOversize() {
        val img = fullImage(size = UpgradePackage.FULL_MAX_BYTES + 512)   // 超用户参数区上界
        assertEquals(Adopt.Reason.IMAGE_SIZE, (UpgradePackage.adopt(img, UpgradePkgType.FULL, 2) as Adopt.Err).reason)
    }

    // ---- 无头差分与未选类型 ----

    @Test
    fun testRawDiffNeedsHeader() {
        val r = UpgradePackage.adopt(ByteArray(500) { 0x55 }, UpgradePkgType.DIFF, 2) as Adopt.Err
        assertEquals(Adopt.Reason.DIFF_NEEDS_HEADER, r.reason)
    }

    @Test
    fun testDiffNeedsHeaderHintsWhenFileLooksLikeFullImage() {
        // 选了差分但文件像全量镜像 → 拒绝并提示核对类型（防呆）
        val r = UpgradePackage.adopt(fullImage(), UpgradePkgType.DIFF, 2) as Adopt.Err
        assertEquals(Adopt.Reason.DIFF_NEEDS_HEADER, r.reason)
        assertTrue(r.detail.contains("疑似全量镜像"))
    }

    @Test
    fun testNoHeaderNoSelectionIsPending() {
        // 非错误：等用户选完类型再校验（UI 据此保持"开始"禁用）
        val r = UpgradePackage.adopt(ByteArray(500) { 0x55 }, null, 2) as Adopt.Err
        assertEquals(Adopt.Reason.TYPE_PENDING, r.reason)
    }

    @Test
    fun testEmptyFile() {
        assertEquals(Adopt.Reason.EMPTY, (UpgradePackage.adopt(ByteArray(0), UpgradePkgType.FULL, 2) as Adopt.Err).reason)
    }
}
