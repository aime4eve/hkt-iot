package com.hkt.ble.bletools.core.protocol

/**
 * LoRaWAN 参数配置协议（0xA0 配置/查询、0xA1 模组状态透传）帧编解码。
 *
 * 协议出处：用户口述（2026-11 对话），固件正本 LoRaWAN_ParkingSensor：
 * - `Compents/LoRa/LoRaWAN_APPLY.c` LoRaWANRegionTable（区域限制，逐字段同源）/
 *   LoRaWAN_CfgValidate（校验规则，App 前置同规则拦截）
 * - `USER/Drive/communicate.c` ble_handle_lorawan_frame（分发）/ callback_BLELoRaWANCfg
 *   （40B 应答帧）/ ble_forward_module_sr（AT+SR0? 裸文本透传）
 *
 * 帧形（新族，与 [UpgradeFrame] 同族但特殊类型恒 0x00）：
 * `hkt(3) 特殊类型(1)=0x00 包序号(1) 数据类型(1) 数据(n) 累加校验(1)`，
 * 校验=自帧头逐字节求和 mod 256。固件不检查特殊类型 BIT 位。
 * - 0xA0 查询：数据 1B=0x01（8B 帧）→ 设备回 40B 配置应答帧（同帧形，数据 33B）
 * - 0xA0 配置：数据 33B（40B 帧）→ 设备校验→写 flash→旧 TLV 0xFF ACK→500ms 后复位重启
 *   （写入 ACK 走旧家族 [HKTResponseParser] 的 0xFF 段，非本帧族）
 * - 0xA1 SR0：数据 1B=0x01（8B 帧）→ 设备回 AT+SR0? 原始裸文本（**非 hkt 帧**）
 *
 * 33B 载荷（大端）：band(0) chmask(1-9, bit0 of byte0 = ch0, 72bit) rx2_dr(10)
 * rx2_freq(11-14, u32BE Hz) adr(15) confirm(16) confirm_cnt(17, 1-9)
 * power(18-19, u16BE 区域相对索引) dr(20-21, u16BE) activation(22, 0=OTAA/1=ABP)
 * class(23, 0=A/2=C) port(24, 1-223) deveui(25-32, 只读——写入时固件忽略).
 *
 * 金向量：shared/fixtures/lorawan-config.json（LW-*，与固件侧实算一致）。
 */

/** 数据类型（指令）码。 */
object LoRaWANMsgType {
    const val CONFIG = 0xA0   // 查询(数据 1B)/配置(数据 33B)，固件按帧长区分
    const val SR0 = 0xA1      // 模组状态透传（AT+SR0?）
}

/** 区域限制表条目（固件 LoRaWANRegionTable 逐字段同源；功率为区域相对索引，dBm 换算由模组裁决）。 */
data class LoRaWANRegion(
    val band: Int,
    val name: String,
    val drMax: Int,
    val drDefault: Int,
    val rx2DrDefault: Int,
    val rx2FreqDefaultHz: Long,
    val powerMax: Int,
    /** 位图内合法信道位数（协议'-'约定）：CN470 全 72 位、US915/AU915 上行 64 位、其余 8 位。 */
    val chValidBits: Int,
    /** 出厂默认信道位图（u16，bit0=ch0）。 */
    val chmaskDefault: Int,
    val freqMinHz: Long,
    val freqMaxHz: Long,
)

object LoRaWANRegions {
    val all: List<LoRaWANRegion> = listOf(
        LoRaWANRegion(6, "CN470", 6, 3, 3, 505_300_000L, 7, 72, 0x00FF, 470_000_000L, 510_000_000L),
        LoRaWANRegion(7, "CN470B", 6, 3, 3, 505_300_000L, 7, 72, 0x00FF, 470_000_000L, 510_000_000L),
        LoRaWANRegion(84, "RU864", 6, 5, 0, 869_100_000L, 7, 8, 0x0003, 860_000_000L, 930_000_000L),
        LoRaWANRegion(85, "IN865", 5, 5, 0, 866_550_000L, 10, 8, 0x0007, 860_000_000L, 930_000_000L),
        LoRaWANRegion(86, "EU868", 6, 5, 0, 869_525_000L, 7, 8, 0x0007, 860_000_000L, 930_000_000L),
        LoRaWANRegion(90, "US915", 4, 3, 8, 923_300_000L, 14, 64, 0x00FF, 860_000_000L, 930_000_000L),
        LoRaWANRegion(91, "AU915", 6, 4, 8, 923_300_000L, 14, 64, 0x00FF, 860_000_000L, 930_000_000L),
        LoRaWANRegion(92, "KR920", 5, 5, 0, 921_900_000L, 7, 8, 0x0007, 860_000_000L, 930_000_000L),
        LoRaWANRegion(93, "AS923-1", 6, 5, 2, 923_200_000L, 7, 8, 0x0003, 860_000_000L, 930_000_000L),
        LoRaWANRegion(94, "AS923-2", 6, 5, 2, 921_400_000L, 7, 8, 0x0003, 860_000_000L, 930_000_000L),
        LoRaWANRegion(95, "AS923-3", 6, 5, 2, 916_600_000L, 7, 8, 0x0003, 860_000_000L, 930_000_000L),
        LoRaWANRegion(96, "AS923-4", 6, 5, 2, 917_300_000L, 7, 8, 0x0003, 860_000_000L, 930_000_000L),
    )

    fun lookup(band: Int): LoRaWANRegion? = all.firstOrNull { it.band == band }
}

/**
 * 功率索引 → dBm 映射（UI 直显 dBm 用；协议仍传输区域相对索引，换算只发生在展示层）。
 *
 * 真源 = 固件 LoRaWANRegionTable 注释。等差区域按「最大值 - 2*index」展开；IN865 前 5 档
 * 同为 22dBm 为显式逐值表；US915/AU915 固件注释 "22..2 dBm, 15 steps" 与等差自相矛盾
 * （22-2*14=-6），按 LoRaWAN RP002 标准 TXPower 表 30..2 步进 2 推定——**待真机核对**
 * （若模组实为 22..2 共 11 档 + 4 档低位，仅改本表即可）。
 */
object LoRaWANPowerTable {
    private val linear: Map<Int, List<Int>> = buildMap {
        put(6, (0..7).map { 17 - 2 * it })                          // CN470 17..3
        put(7, (0..7).map { 17 - 2 * it })                          // CN470B 同 CN470
        put(85, listOf(22, 22, 22, 22, 22, 20, 18, 16, 14, 12, 10)) // IN865 显式表
        put(90, (0..14).map { 30 - 2 * it })                        // US915 30..2（推定）
        put(91, (0..14).map { 30 - 2 * it })                        // AU915 同 US915
        put(92, (0..7).map { 14 - 2 * it })                         // KR920 14..0
    }

    fun dbmList(band: Int): List<Int> =
        linear[band] ?: (0..7).map { 16 - 2 * it }                  // RU864/EU868/AS923 16..2

    /** 索引越界（异常防御）返回 null，UI 回退显示索引原值。 */
    fun dbmOf(band: Int, index: Int): Int? = dbmList(band).getOrNull(index)
}

/** 一份 LoRaWAN 链路配置 = 0xA0 的 33B 载荷。 */
data class LoRaWANConfig(
    val band: Int,               // AT+BAND 码，见 [LoRaWANRegions]
    val chmask: ByteArray,       // 9B 信道位图，bit0 of byte0 = ch0（必须 9B）
    val rx2Dr: Int,              // 0..region.drMax
    val rx2FreqHz: Long,         // u32BE Hz，须落在区域频段内
    val adr: Int,                // 0/1
    val confirm: Int,            // 0/1
    val confirmCnt: Int,         // 1-9
    val power: Int,              // u16BE 区域相对索引 0..region.powerMax
    val dr: Int,                 // u16BE 0..region.drMax
    val activation: Int,         // 0=OTAA 1=ABP（ABP 密钥固件内置，协议不可配）
    val deviceClass: Int,        // 0=A 2=C
    val port: Int,               // 1-223
    val deveui: ByteArray,       // 8B 只读：查询上报，写入被固件忽略
) {
    fun encode(): ByteArray {
        require(chmask.size == 9) { "chmask must be 9 bytes" }
        require(deveui.size == 8) { "deveui must be 8 bytes" }
        val out = ByteArray(PAYLOAD_LEN)
        out[0] = band.toByte()
        chmask.copyInto(out, 1)
        out[10] = rx2Dr.toByte()
        u32be(rx2FreqHz).copyInto(out, 11)
        out[15] = adr.toByte()
        out[16] = confirm.toByte()
        out[17] = confirmCnt.toByte()
        u16be(power).copyInto(out, 18)
        u16be(dr).copyInto(out, 20)
        out[22] = activation.toByte()
        out[23] = deviceClass.toByte()
        out[24] = port.toByte()
        deveui.copyInto(out, 25)
        return out
    }

    override fun equals(other: Any?): Boolean =
        other is LoRaWANConfig && other.band == band && other.chmask.contentEquals(chmask) &&
            other.rx2Dr == rx2Dr && other.rx2FreqHz == rx2FreqHz && other.adr == adr &&
            other.confirm == confirm && other.confirmCnt == confirmCnt && other.power == power &&
            other.dr == dr && other.activation == activation && other.deviceClass == deviceClass &&
            other.port == port && other.deveui.contentEquals(deveui)

    override fun hashCode(): Int {
        var h = band
        h = 31 * h + chmask.contentHashCode()
        h = 31 * h + rx2Dr
        h = 31 * h + rx2FreqHz.hashCode()
        h = 31 * h + adr
        h = 31 * h + confirm
        h = 31 * h + confirmCnt
        h = 31 * h + power
        h = 31 * h + dr
        h = 31 * h + activation
        h = 31 * h + deviceClass
        h = 31 * h + port
        h = 31 * h + deveui.contentHashCode()
        return h
    }

    companion object {
        const val PAYLOAD_LEN = 33

        fun decode(payload: ByteArray): LoRaWANConfig? {
            if (payload.size != PAYLOAD_LEN) return null
            return LoRaWANConfig(
                band = payload[0].toInt() and 0xFF,
                chmask = payload.copyOfRange(1, 10),
                rx2Dr = payload[10].toInt() and 0xFF,
                rx2FreqHz = BEValue.u32(payload, 11).toLong() and 0xFFFF_FFFFL,
                adr = payload[15].toInt() and 0xFF,
                confirm = payload[16].toInt() and 0xFF,
                confirmCnt = payload[17].toInt() and 0xFF,
                power = BEValue.u16(payload, 18),
                dr = BEValue.u16(payload, 20),
                activation = payload[22].toInt() and 0xFF,
                deviceClass = payload[23].toInt() and 0xFF,
                port = payload[24].toInt() and 0xFF,
                deveui = payload.copyOfRange(25, 33),
            )
        }

        /** 区域默认配置（固件 LoRaWAN_CfgSetDefaults 同源：adr=1/confirm=1/cnt=3/power=0/OTAA）。 */
        fun defaults(region: LoRaWANRegion, deviceClass: Int = 0, port: Int = 10): LoRaWANConfig =
            LoRaWANConfig(
                band = region.band,
                chmask = ChmaskCodec.fromU16(region.chmaskDefault),
                rx2Dr = region.rx2DrDefault,
                rx2FreqHz = region.rx2FreqDefaultHz,
                adr = 1,
                confirm = 1,
                confirmCnt = 3,
                power = 0,
                dr = region.drDefault,
                activation = 0,
                deviceClass = deviceClass,
                port = port,
                deveui = ByteArray(8),
            )
    }
}

/**
 * 信道位图 ↔ 十六进制文本（UI 输入/显示）。**字节序直读（2026-11-14 用户真机裁决）**：
 * hex 串与 33B 载荷/模组 AT+CHMASK 字节序一致——**左起第一字节 = ch0–7**，第二字节 = ch8–15，
 * 最长 18 位（72bit 全开 = FFFFFFFFFFFFFFFFFFF）。显示自左（ch0）向右、裁右侧高位零字节、
 * 最少 4 位：CN470 默认 [FF,00..] → "FF00"；EU868 [07,00..] → "0700"；"0007" 表示 ch8–10
 * 开（8 信道区域即越界）。历史 %04X 数值形态（BE 显示）已废弃。
 */
object ChmaskCodec {
    const val MAX_HEX_DIGITS = 18   // 72 bit

    /** 十六进制文本 → 9B 位图（左字节=chmask[0]=ch0–7；奇数长度左补 0）。null = 非法字符/超长/空。 */
    fun parseHex(text: String): ByteArray? {
        val t = text.trim()
        if (t.isEmpty() || t.length > MAX_HEX_DIGITS) return null
        if (!t.all { it in "0123456789abcdefABCDEF" }) return null
        val padded = if (t.length % 2 == 1) "0$t" else t
        val out = ByteArray(9)
        for (i in 0 until padded.length / 2) {
            out[i] = padded.substring(i * 2, i * 2 + 2).toInt(16).toByte()
        }
        return out
    }

    /** 9B 位图 → 显示串（chmask[0] 最左、大写、最少 4 位）。 */
    fun toDisplay(mask: ByteArray): String {
        require(mask.size == 9)
        var top = 8
        while (top > 0 && mask[top].toInt() == 0) top--
        if (top < 1) top = 1   // 至少两个字节 = 4 个十六进制位
        return buildString {
            for (i in 0..top) append("%02X".format(mask[i]))
        }
    }

    /** u16 位图（bit0=ch0）→ 9B 位图。低字节落 chmask[0]，与 [parseHex] 字节序一致。 */
    fun fromU16(value: Int): ByteArray {
        val out = ByteArray(9)
        out[0] = (value and 0xFF).toByte()
        out[1] = ((value shr 8) and 0xFF).toByte()
        return out
    }
}

/** APP 上行帧编码（特殊类型恒 0x00，与升级帧的 0x0B 区分）。 */
object LoRaWANFrame {
    val prefix = byteArrayOf(0x68, 0x6B, 0x74)

    fun queryFrame(seq: Int): ByteArray = frame(seq, LoRaWANMsgType.CONFIG, byteArrayOf(0x01))
    fun sr0Frame(seq: Int): ByteArray = frame(seq, LoRaWANMsgType.SR0, byteArrayOf(0x01))

    /** 0x8E 电量校准：data=百分比 0–100（100=换新电池归零校准）。固件反算计数落盘后回 0xFF ACK。 */
    fun batteryCalibFrame(seq: Int, percent: Int): ByteArray =
        frame(seq, 0x8E, byteArrayOf(percent.coerceIn(0, 100).toByte()))

    /** 0xA2 BLE 调试模式（17B 定长帧，仅蓝牙）：设备类型 0=地磁 / 调试开关 / 连接允许睡眠 / 预留 7B。 */
    fun bleDebugFrame(seq: Int, debugOn: Boolean, sleepOn: Boolean): ByteArray =
        frame(seq, 0xA2, byteArrayOf(0, if (debugOn) 1 else 0, if (sleepOn) 1 else 0, 0, 0, 0, 0, 0, 0, 0))
    fun configFrame(seq: Int, cfg: LoRaWANConfig): ByteArray =
        frame(seq, LoRaWANMsgType.CONFIG, cfg.encode())

    private fun frame(seq: Int, type: Int, data: ByteArray): ByteArray {
        val out = ByteArray(prefix.size + 1 + 1 + 1 + data.size + 1)
        var o = 0
        prefix.copyInto(out, o); o += prefix.size
        out[o++] = 0x00   // 特殊类型恒 0x00（固件 ble_handle_lorawan_frame 不检查 BIT 位）
        out[o++] = seq.toByte()
        out[o++] = type.toByte()
        data.copyInto(out, o); o += data.size
        out[o] = UpgradeFrame.checksum(out, o).toByte()
        return out
    }
}

/**
 * 固件 [LoRaWANConfig.validate] 的镜像（LoRaWAN_APPLY.c LoRaWAN_CfgValidate）：任一非法
 * 固件整包静默拒绝（无 ACK），App 侧前置同规则拦截。返回 null = 合法，否则返回违规字段
 * key（UI 行内错误锚点，取第一条违规）。
 */
object LoRaWANValidation {
    /** RX2 DR 全域硬上限：下行接收窗口速率与上行 drMax 无关（US915 默认 RX2 DR8 而 drMax=4），
     *  13 = LoRaWAN 区域参数下行 DR 理论最大；具体区域是否接受由模组最终裁决。 */
    const val RX2_DR_MAX = 13

    fun errorKey(cfg: LoRaWANConfig): String? {
        val region = LoRaWANRegions.lookup(cfg.band) ?: return "band"
        if (cfg.rx2Dr > RX2_DR_MAX) return "rx2Dr"
        if (cfg.rx2FreqHz < region.freqMinHz || cfg.rx2FreqHz > region.freqMaxHz) return "rx2Freq"
        if (cfg.adr > 1) return "adr"
        if (cfg.confirm > 1) return "confirm"
        if (cfg.confirmCnt < 1 || cfg.confirmCnt > 9) return "confirmCnt"
        if (cfg.power > region.powerMax) return "power"
        if (cfg.dr > region.drMax) return "dr"
        if (cfg.activation > 1) return "activation"
        if (cfg.deviceClass != 0 && cfg.deviceClass != 2) return "deviceClass"
        if (cfg.port < 1 || cfg.port > 223) return "port"
        var anyChannel = false
        for (i in 0 until 72) {
            val bitSet = ((cfg.chmask[i / 8].toInt() and 0xFF) shr (i % 8)) and 1 == 1
            if (i < region.chValidBits) {
                if (bitSet) anyChannel = true
            } else if (bitSet) {
                return "chmask"   // 越界位：区域未定义该信道
            }
        }
        if (!anyChannel) return "chmask"   // 全关：至少保留 1 个合法信道
        return null
    }
}

/**
 * SR0（LW STATUS0，0xA1 应答内容）40B 状态解析。真源 = 模组手册表 5-23/5-24（用户 2026-11-14
 * 提供截图）：查询 opcode 0x1402、N=40；应答参数区 = Type 04 / Event 0E / ResultLen 4+40 /
 * RFU / OpCode（LE：02 14）/ ResultCode / 40B STATUS0。**多字节字段一律小端**——RSSI 示例
 * -87dBm → Data[13]=0xA9 Data[14]=0xFF；DevNonce/TOA 按小端读与示例场景自洽（按大端读
 * 出 56 分钟 TOA 等不合理值）。
 */
data class LoRaWANSr0Status(
    val commResult: Int,       // 表 5-24 offset0：0-8 枚举
    val uplinkCh: Int,
    val uplinkDr: Int,
    val uplinkPowerDbm: Int,
    val downlinkCh: Int,
    val rx1Dr: Int,            // offset5 低 4 位
    val rx1DrOffset: Int,      // offset5 高 4 位
    val rx2Dr: Int,
    val rxSlot: Int,
    val fport: Int,
    val gateways: Int,
    val snrDb: Int,            // int8 补码
    val demodMarginDb: Int,
    val rssiDbm: Int,          // int16 LE 补码
    val downFcnt: Long,        // u32 LE
    val upFcnt: Long,
    val joinCount: Int,        // u16 LE
    val devNonce: Int,         // u16 LE
    val toaOverflowDown: Int,  // offset27 高 4 位
    val toaOverflowUp: Int,    // offset27 低 4 位
    val downToaUs: Long,       // u32 LE
    val upToaUs: Long,         // u32 LE
) {
    private fun commResultText(zh: Boolean): String {
        val zhText = listOf(
            "无数据操作", "数据发送中", "数据发送失败", "发送成功（非确认，无下行）", "JOIN 失败",
            "JOIN 成功", "网络可能异常", "发送成功，无下行", "发送成功，有下行",
        )
        val enText = listOf(
            "no data op", "sending", "send failed", "sent (unconf, no DL)", "join failed",
            "join OK", "network abnormal", "sent, no DL", "sent, with DL",
        )
        return (if (zh) zhText else enText).getOrNull(commResult) ?: "#$commResult"
    }

    /** UI 展示行（分组聚合，RFU 丢弃）。 */
    fun rows(zh: Boolean): List<Pair<String, String>> = buildList {
        add((if (zh) "通信结果" else "Comm result") to "$commResult · ${commResultText(zh)}")
        add((if (zh) "上行" else "Uplink") to "CH$uplinkCh · DR$uplinkDr · $uplinkPowerDbm dBm")
        add((if (zh) "下行" else "Downlink") to "CH$downlinkCh · RX1 DR$rx1Dr+$rx1DrOffset · RX2 DR$rx2Dr · SLOT$rxSlot · FPORT$fport")
        add((if (zh) "可用网关" else "Gateways") to "$gateways")
        add("SNR / RSSI" to "$snrDb dB / $rssiDbm dBm")
        add("DemodMargin" to "$demodMarginDb dB")
        add("FCNT" to (if (zh) "下行 $downFcnt · 上行 $upFcnt" else "DL $downFcnt · UL $upFcnt"))
        add("JOIN" to (if (zh) "累计 $joinCount · DevNonce $devNonce" else "count $joinCount · DevNonce $devNonce"))
        add("TOA" to (if (zh) "下行 $downToaUs µs · 上行 $upToaUs µs" else "DL $downToaUs µs · UL $upToaUs µs"))
    }
}

object LoRaWANSr0Parser {
    private val HEX_RUN = Regex("[0-9A-Fa-f]+")

    /**
     * 从 SR0 裸文本提取并解析 STATUS0。识别两种形态：
     * 1. 含应答帧头：`04 0E xx xx (02 14|14 02) xx <40B>`（手册表 5-23 参数区，ResultCode 后取 40B）
     * 2. 纯 40 字节 hex（模组 AT 应答直给状态体）
     * hex 提取规则：任意非 hex 字符为分隔（空格/逗号/换行/`+SR0:`等），仅收长度为偶数的段，
     * 长度 1 的段（如 "SR0" 的 0）丢弃。无法定位 40B 时返回 null（UI 只显原文）。
     */
    fun parse(text: String): LoRaWANSr0Status? {
        val bytes = HEX_RUN.findAll(text)
            .mapNotNull { run -> run.value.takeIf { it.length % 2 == 0 } }
            .flatMap { hex -> hex.chunked(2).map { it.toInt(16).toByte() } }
            .toList().toByteArray()
        // 形态 1：应答参数区 Type=04 Event=0E ... OpCode(LE 02 14) ResultCode <40B>
        for (i in 0..bytes.size - 7) {
            if (bytes[i] == 0x04.toByte() && bytes[i + 1] == 0x0E.toByte()) {
                val dataStart = i + 7   // Type(1)Event(1)Len(1)RFU(1)OpCode(2)Result(1)
                val opMatch = (bytes[i + 4] == 0x02.toByte() && bytes[i + 5] == 0x14.toByte()) ||
                    (bytes[i + 4] == 0x14.toByte() && bytes[i + 5] == 0x02.toByte())
                if (opMatch && dataStart + 40 <= bytes.size) {
                    return decode(bytes.copyOfRange(dataStart, dataStart + 40))
                }
            }
        }
        // 形态 2：恰 40 字节状态体
        if (bytes.size == 40) return decode(bytes)
        return null
    }

    private fun decode(b: ByteArray): LoRaWANSr0Status {
        fun u8(i: Int) = b[i].toInt() and 0xFF
        fun u16le(i: Int) = u8(i) or (u8(i + 1) shl 8)
        fun u32le(i: Int) = (u16le(i).toLong() and 0xFFFF) or ((u16le(i + 2).toLong() and 0xFFFF) shl 16)
        return LoRaWANSr0Status(
            commResult = u8(0),
            uplinkCh = u8(1),
            uplinkDr = u8(2),
            uplinkPowerDbm = u8(3),
            downlinkCh = u8(4),
            rx1DrOffset = (u8(5) shr 4) and 0x0F,
            rx1Dr = u8(5) and 0x0F,
            rx2Dr = u8(6),
            rxSlot = u8(7),
            fport = u8(8),
            gateways = u8(10),
            snrDb = b[11].toInt(),                                   // int8 符号扩展
            demodMarginDb = b[12].toInt(),
            rssiDbm = b[13].toInt() or (b[14].toInt() shl 8),         // int16 LE 符号扩展
            downFcnt = u32le(15),
            upFcnt = u32le(19),
            joinCount = u16le(23),
            devNonce = u16le(25),
            toaOverflowDown = (u8(27) shr 4) and 0x0F,
            toaOverflowUp = u8(27) and 0x0F,
            downToaUs = u32le(28),
            upToaUs = u32le(32),
        )
    }
}

/**
 * 设备→APP 入站帧（0xA0 配置应答，40B）。严格识别：前缀 + 特殊类型 0x00 + 类型 0xA0 +
 * 定长 40 + 尾累加校验。与升级帧（特殊类型 0x0B / 类型 F0-F3,FE）和 TLV 状态流（无 0xA0
 * 类型）互斥，误识别不可能；在 [com.hkt.ble.bletools.core.ble.DeviceSession] 旁路 TLV
 * 解析（否则 unknownTail 误报"响应数据异常"）。0xA1 应答为裸文本（非 hkt 帧），不经此处。
 */
sealed interface LoRaWANInbound {
    /** 40B 配置应答：[seq] 为设备侧包序号（packSyncNumber），[cfg] 为解出的 33B 配置。 */
    data class ConfigResponse(val seq: Int, val cfg: LoRaWANConfig) : LoRaWANInbound

    companion object {
        private const val LEN_CONFIG_RESPONSE = 40   // hkt(3)+0x00+seq(1)+A0(1)+33B+校验(1)

        fun parse(data: ByteArray): LoRaWANInbound? {
            if (data.size != LEN_CONFIG_RESPONSE) return null
            if (data[0] != LoRaWANFrame.prefix[0] || data[1] != LoRaWANFrame.prefix[1] ||
                data[2] != LoRaWANFrame.prefix[2]
            ) return null
            if (data[3].toInt() and 0xFF != 0x00) return null
            if (data[5].toInt() and 0xFF != LoRaWANMsgType.CONFIG) return null
            if (UpgradeFrame.checksum(data, data.size - 1) != (data[data.size - 1].toInt() and 0xFF)) return null
            val cfg = LoRaWANConfig.decode(data.copyOfRange(6, 39)) ?: return null
            return ConfigResponse(data[4].toInt() and 0xFF, cfg)
        }
    }
}
