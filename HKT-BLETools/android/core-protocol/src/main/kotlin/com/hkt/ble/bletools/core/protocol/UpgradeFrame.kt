package com.hkt.ble.bletools.core.protocol

/**
 * 新升级协议（差分/全量升级）帧编解码。协议出处：《蓝牙升级协议.docx》（2026-11，地磁 DC200 新固件）。
 *
 * 帧形：`hkt(3) 特殊类型(1) 包序号(1) 数据类型(1) 数据(n) 校验(1)` —— 与本仓库既有两种帧形
 * （app 帧 `hkt packnum len cmd data crc16` / 引导层帧 `hkt len cmd packnum data crc bootload`）
 * 均不兼容，物理上是三套并存协议（新升级仅在设备进入升级态后使用）。
 *
 * - 特殊类型按 BIT 位定义：BIT0 需应答 / BIT1 带校验 / BIT2 历史数据 / BIT3 升级类指令
 *   （升级类强制带校验 → 升级帧统一 0x0B）；设备上行 0xFE/0xF3 示例为 0x02（仅校验）。
 * - 校验：自帧头（含同步头）起逐字节求和 mod 256，1 字节 —— 不是 CRC。
 * - 多字节字段大端（版本号/总包数/每包长度/包序号/CRC32/CRC16）。
 *
 * 金向量：shared/fixtures/upgrade-protocol.json（UPG-*，取自协议文档完整升级示例）。
 */

/** 特殊类型字节 BIT 位。 */
object UpgradeFlags {
    const val NEED_ACK = 0x01   // BIT0：需应答/确认包
    const val CHECKSUM = 0x02   // BIT1：帧尾带累加校验
    const val HISTORY = 0x04    // BIT2：历史数据
    const val UPGRADE = 0x08    // BIT3：升级类指令（强制同时置 BIT1）

    /** APP 上行升级帧（0xF0/0xF1）统一特殊类型：BIT0|BIT1|BIT3（文档示例均为 0x0B）。 */
    const val UPGRADE_FRAME = NEED_ACK or CHECKSUM or UPGRADE
}

/** 数据类型（指令）码。 */
object UpgradeMsgType {
    const val NOTIFY = 0xF0   // APP→设备 新版本通知（11B，设备须回 0xFE 应答）
    const val DATA = 0xF1     // APP→设备 升级包（6+N，被动：设备 0xF2 按序号拉取）
    const val REQUEST = 0xF2  // 设备→APP 请求升级包（2B 包序号）
    const val RESULT = 0xF3   // 设备→APP 升级结果上报（2B：结果码+附加信息）
    const val ACK = 0xFE      // 通用应答（1B：被应答的数据类型）
}

/** 0xF0 第 7 字节包类型：全量包兜底救砖，差分包为常规升级路径。 */
object UpgradePkgType {
    const val FULL = 0x00   // 全量：设备直接当 NewFW 走搬运链（底包损坏/版本不明时的救援手段）
    const val DIFF = 0x01   // 差分：设备 APP 区内合成新固件到外部 NewFW 区
}

/** 0xF3 结果码。 */
object UpgradeResultCode {
    const val SUCCESS = 0x00
    const val CRC_FAILED = 0x01   // 校验失败（包 CRC / 整包 CRC / 差分底包不匹配）
    const val WRITE_FAILED = 0x02 // 写入失败
    const val TIMEOUT = 0x03      // 设备等待数据超时
}

object UpgradeFrame {
    val prefix = byteArrayOf(0x68, 0x6B, 0x74)

    /** 尾校验：自同步头起逐字节求和 mod 256（[upTo] 为不含校验字节的长度）。 */
    fun checksum(frame: ByteArray, upTo: Int): Int {
        var sum = 0
        for (i in 0 until upTo) sum = (sum + (frame[i].toInt() and 0xFF)) and 0xFF
        return sum
    }

    private fun frame(seq: Int, type: Int, special: Int, data: ByteArray): ByteArray {
        val out = ByteArray(prefix.size + 1 + 1 + 1 + data.size + 1)
        var o = 0
        prefix.copyInto(out, o); o += prefix.size
        out[o++] = special.toByte()
        out[o++] = seq.toByte()
        out[o++] = type.toByte()
        data.copyInto(out, o); o += data.size
        out[o] = checksum(out, o).toByte()
        return out
    }

    /**
     * 0xF0 新版本通知（数据定长 11B）：
     * 版本号 2（高字节硬件版本、低字节软件版本）+ 总包数 2 + 每包长度 2（默认 0x0100）
     * + 包类型 1（[UpgradePkgType]）+ 更新文件 CRC32 4。
     */
    fun notifyFrame(
        seq: Int,
        hwVersion: Int,
        swVersion: Int,
        totalPackets: Int,
        packetLen: Int,
        pkgType: Int,
        fileCrc32: Long,
    ): ByteArray = frame(
        seq, UpgradeMsgType.NOTIFY, UpgradeFlags.UPGRADE_FRAME,
        byteArrayOf(hwVersion.toByte(), swVersion.toByte()) +
            u16be(totalPackets) + u16be(packetLen) + byteArrayOf(pkgType.toByte()) + u32be(fileCrc32),
    )

    /**
     * 0xF1 升级包下发（数据 6+N）：包序号 2 + 包数据 CRC16/CCITT 2 + 包长 2 + 包数据 N。
     * 包 CRC 取固件 crclib.c 同款 KERMIT 反射实现（[CRC16.ccitt]）；文档仅写 "CRC16/CCITT"，
     * 与设备实现的一致性属真机联调核对项。
     */
    fun dataFrame(seq: Int, packetIndex: Int, chunk: ByteArray): ByteArray = frame(
        seq, UpgradeMsgType.DATA, UpgradeFlags.UPGRADE_FRAME,
        u16be(packetIndex) + u16be(CRC16.ccitt(chunk)) + u16be(chunk.size) + chunk,
    )
}

/**
 * 设备→APP 入站帧（0xF2/0xF3/0xFE）。严格识别：前缀 + 定长 + 尾校验有效，
 * 类型判据与三家族 TLV 表（[TLVSizeTable]）天然互斥（F2/F3/FE 均非合法 TLV 类型），
 * 旧固件状态流不可能被误识别。
 */
sealed interface UpgradeInbound {
    /** 0xF2 设备请求第 [packetIndex] 包。 */
    data class PacketRequest(val packetIndex: Int) : UpgradeInbound

    /** 0xF3 升级结果：[code] 见 [UpgradeResultCode]，[extra] 为附加信息。 */
    data class ResultReport(val code: Int, val extra: Int) : UpgradeInbound

    /** 0xFE 通用应答：确认收到类型为 [ackedType] 的帧。 */
    data class GenericAck(val ackedType: Int) : UpgradeInbound

    companion object {
        private const val LEN_REQUEST = 9   // hkt(3)+特殊(1)+序号(1)+F2(1)+数据(2)+校验(1)
        private const val LEN_RESULT = 9    // hkt(3)+特殊(1)+序号(1)+F3(1)+数据(2)+校验(1)
        private const val LEN_ACK = 8       // hkt(3)+特殊(1)+序号(1)+FE(1)+数据(1)+校验(1)

        /** 识别并解析一条入站升级帧；结构/校验不符返回 null（调用方据此分流或放弃）。 */
        fun parse(data: ByteArray): UpgradeInbound? {
            if (data.size < LEN_ACK) return null
            if (data[0] != UpgradeFrame.prefix[0] || data[1] != UpgradeFrame.prefix[1] ||
                data[2] != UpgradeFrame.prefix[2]
            ) return null
            if (!checksumOk(data)) return null
            return when (data[5].toInt() and 0xFF) {
                UpgradeMsgType.REQUEST ->
                    if (data.size != LEN_REQUEST) null
                    else PacketRequest(((data[6].toInt() and 0xFF) shl 8) or (data[7].toInt() and 0xFF))
                UpgradeMsgType.RESULT ->
                    if (data.size != LEN_RESULT) null
                    else ResultReport(data[6].toInt() and 0xFF, data[7].toInt() and 0xFF)
                UpgradeMsgType.ACK ->
                    if (data.size != LEN_ACK) null
                    else GenericAck(data[6].toInt() and 0xFF)
                else -> null
            }
        }

        private fun checksumOk(data: ByteArray): Boolean =
            UpgradeFrame.checksum(data, data.size - 1) == (data[data.size - 1].toInt() and 0xFF)
    }
}
