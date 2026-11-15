package com.hkt.ble.bletools.core.protocol

import com.hkt.ble.bletools.core.protocol.Fixtures.bytesToHex
import com.hkt.ble.bletools.core.protocol.Fixtures.hexToBytes
import com.hkt.ble.bletools.core.protocol.Fixtures.jsonArray
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNull
import kotlin.test.assertTrue
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive
import kotlinx.serialization.json.int
import kotlinx.serialization.json.long

/// 0xA0/0xA1 LoRaWAN 配置协议测试（真源 = shared/fixtures/lorawan-config.json，向量与固件侧
/// 实算一致：查询尾 E9 / SR0 尾 EA / 配置（869.525M 示例）尾 B2；区域表与校验镜像
/// LoRaWAN_APPLY.c LoRaWANRegionTable / LoRaWAN_CfgValidate）。
class LoRaWANConfigTests {

    private fun vector(id: String) =
        jsonArray("lorawan-config.json").map { it.jsonObject }
            .first { it["id"]!!.jsonPrimitive.content == id }

    private fun request(id: String) = hexToBytes(vector(id)["request"]!!.jsonPrimitive.content)
    private fun response(id: String) = hexToBytes(vector(id)["response"]!!.jsonPrimitive.content)

    /** LW-CONFIG-001 / LW-RSP-CONFIG-001 的 33B 载荷等价配置对象。 */
    private fun goldenConfig() = LoRaWANConfig(
        band = 86,
        chmask = ChmaskCodec.fromU16(0x0007),
        rx2Dr = 1,
        rx2FreqHz = 869_525_000L,
        adr = 1,
        confirm = 1,
        confirmCnt = 3,
        power = 0,
        dr = 5,
        activation = 0,
        deviceClass = 0,
        port = 10,
        deveui = hexToBytes("1122334455667788"),
    )

    @Test
    fun testQueryFrameGolden() {
        assertEquals(bytesToHex(request("LW-QUERY-001")), bytesToHex(LoRaWANFrame.queryFrame(1)))
    }

    @Test
    fun testSr0FrameGolden() {
        assertEquals(bytesToHex(request("LW-SR0-001")), bytesToHex(LoRaWANFrame.sr0Frame(1)))
    }

    @Test
    fun testBatteryCalibFrameGolden() {
        // 用户定义帧例：68 6B 74 00 01 8E 64（校准 100%，换新电池归零）
        assertEquals(bytesToHex(request("LW-BATCAL-001")), bytesToHex(LoRaWANFrame.batteryCalibFrame(1, 100)))
        // 百分比越界防御（coerceIn 0..100）
        assertEquals(
            bytesToHex(LoRaWANFrame.batteryCalibFrame(1, 100)),
            bytesToHex(LoRaWANFrame.batteryCalibFrame(1, 120)),
        )
    }

    @Test
    fun testBleDebugFrameGolden() {
        // 用户定义帧例：68 6B 74 00 01 A2 00 01 01 00..00（地磁·开调试·连接允许睡眠，17B 定长）
        assertEquals(
            bytesToHex(request("LW-BLEDBG-001")),
            bytesToHex(LoRaWANFrame.bleDebugFrame(seq = 1, debugOn = true, sleepOn = true)),
        )
        // 全关形态：帧长恒 17B，data[7]/data[8] 随开关
        val off = LoRaWANFrame.bleDebugFrame(seq = 2, debugOn = false, sleepOn = false)
        assertEquals(17, off.size)
        assertEquals(0x00, off[7].toInt() and 0xFF)
        assertEquals(0x00, off[8].toInt() and 0xFF)
    }

    @Test
    fun testConfigFrameGoldenAndRoundtrip() {
        val frame = LoRaWANFrame.configFrame(1, goldenConfig())
        assertEquals(bytesToHex(request("LW-CONFIG-001")), bytesToHex(frame))
        // 33B 载荷 encode → decode 无损往返
        assertEquals(goldenConfig(), LoRaWANConfig.decode(frame.copyOfRange(6, 39)))
    }

    @Test
    fun testDecodeRejectsWrongLength() {
        assertNull(LoRaWANConfig.decode(ByteArray(32)))
        assertNull(LoRaWANConfig.decode(ByteArray(34)))
    }

    @Test
    fun testParseConfigResponse() {
        val expected = vector("LW-RSP-CONFIG-001")["expected"]!!.jsonObject
        val inbound = LoRaWANInbound.parse(response("LW-RSP-CONFIG-001"))
        assertTrue(inbound is LoRaWANInbound.ConfigResponse)
        val cfg = inbound.cfg
        assertEquals(expected["seq"]!!.jsonPrimitive.int, inbound.seq)
        assertEquals(expected["band"]!!.jsonPrimitive.int, cfg.band)
        assertEquals(expected["chmaskDisplay"]!!.jsonPrimitive.content, ChmaskCodec.toDisplay(cfg.chmask))
        assertEquals(expected["rx2Dr"]!!.jsonPrimitive.int, cfg.rx2Dr)
        assertEquals(expected["rx2FreqHz"]!!.jsonPrimitive.long, cfg.rx2FreqHz)
        assertEquals(expected["adr"]!!.jsonPrimitive.int, cfg.adr)
        assertEquals(expected["confirm"]!!.jsonPrimitive.int, cfg.confirm)
        assertEquals(expected["confirmCnt"]!!.jsonPrimitive.int, cfg.confirmCnt)
        assertEquals(expected["power"]!!.jsonPrimitive.int, cfg.power)
        assertEquals(expected["dr"]!!.jsonPrimitive.int, cfg.dr)
        assertEquals(expected["activation"]!!.jsonPrimitive.int, cfg.activation)
        assertEquals(expected["deviceClass"]!!.jsonPrimitive.int, cfg.deviceClass)
        assertEquals(expected["port"]!!.jsonPrimitive.int, cfg.port)
        assertEquals(expected["deveui"]!!.jsonPrimitive.content, bytesToHex(cfg.deveui))
    }

    @Test
    fun testParseRejectsCorruptedChecksum() {
        assertNull(LoRaWANInbound.parse(response("LW-RSP-BADSUM")))
    }

    @Test
    fun testParseRejectsForeignFrames() {
        // 8B 上行帧 / 升级帧 / 旧 TLV 状态流 / 引导层帧均不得被识别为 0xA0 应答（三重判据互斥）
        assertNull(LoRaWANInbound.parse(LoRaWANFrame.queryFrame(1)))
        assertNull(LoRaWANInbound.parse(UpgradeFrame.notifyFrame(1, 0x60, 1, 2, 4, 0, 0x12345678L)))
        assertNull(LoRaWANInbound.parse(UpgradeFrame.dataFrame(1, 0, ByteArray(240))))
        assertNull(LoRaWANInbound.parse(hexToBytes("686B740005FF00000001")))
        // 反向互斥：0xA0 帧也不得被升级入站解析吞掉
        assertNull(UpgradeInbound.parse(LoRaWANFrame.configFrame(1, goldenConfig())))
    }

    @Test
    fun testRegionTableMirrorsFirmware() {
        assertEquals(12, LoRaWANRegions.all.size)
        val eu = LoRaWANRegions.lookup(86)!!
        assertEquals("EU868", eu.name)
        assertEquals(6, eu.drMax)
        assertEquals(5, eu.drDefault)
        assertEquals(0, eu.rx2DrDefault)
        assertEquals(869_525_000L, eu.rx2FreqDefaultHz)
        assertEquals(7, eu.powerMax)
        assertEquals(8, eu.chValidBits)
        assertEquals(0x0007, eu.chmaskDefault)
        assertEquals(860_000_000L, eu.freqMinHz)
        assertEquals(930_000_000L, eu.freqMaxHz)
        // 宽位图区域（2026-11-14 用户裁决：CHMASK 不限 u16）
        assertEquals(72, LoRaWANRegions.lookup(6)!!.chValidBits)
        assertEquals(72, LoRaWANRegions.lookup(7)!!.chValidBits)
        assertEquals(64, LoRaWANRegions.lookup(90)!!.chValidBits)
        assertEquals(64, LoRaWANRegions.lookup(91)!!.chValidBits)
        assertEquals(10, LoRaWANRegions.lookup(85)!!.powerMax)
        assertEquals(14, LoRaWANRegions.lookup(90)!!.powerMax)
        assertNull(LoRaWANRegions.lookup(88))
        assertNull(LoRaWANRegions.lookup(0))
    }

    @Test
    fun testDefaultsMatchFirmwareSetDefaults() {
        val d = LoRaWANConfig.defaults(LoRaWANRegions.lookup(86)!!)
        assertNull(LoRaWANValidation.errorKey(d))
        assertEquals(0, d.activation)     // OTAA
        assertEquals(1, d.adr)
        assertEquals(1, d.confirm)
        assertEquals(3, d.confirmCnt)
        assertEquals(0, d.power)
        assertEquals(5, d.dr)
    }

    @Test
    fun testValidationMirrorsFirmwareBranches() {
        fun cfg(
            band: Int = 86, rx2Dr: Int = 0, rx2FreqHz: Long = 869_525_000L, adr: Int = 1,
            confirm: Int = 1, confirmCnt: Int = 3, power: Int = 0, dr: Int = 5,
            activation: Int = 0, deviceClass: Int = 0, port: Int = 10,
            chmask: ByteArray = ChmaskCodec.fromU16(0x0007),
        ) = LoRaWANConfig(band, chmask, rx2Dr, rx2FreqHz, adr, confirm, confirmCnt, power, dr,
            activation, deviceClass, port, ByteArray(8))

        assertNull(LoRaWANValidation.errorKey(cfg()))                                  // 基线合法
        assertEquals("band", LoRaWANValidation.errorKey(cfg(band = 88)))               // 区域不存在
        // RX2 为下行速率，与上行 drMax 无关（2026-11-14 用户裁决）：EU868 drMax=6 下 rx2Dr=8 合法
        assertNull(LoRaWANValidation.errorKey(cfg(rx2Dr = 8)))
        assertNull(LoRaWANValidation.errorKey(cfg(rx2Dr = 13)))
        assertEquals("rx2Dr", LoRaWANValidation.errorKey(cfg(rx2Dr = 14)))             // > 全域下行上限 13
        assertEquals("rx2Freq", LoRaWANValidation.errorKey(cfg(rx2FreqHz = 940_000_000L)))
        assertEquals("rx2Freq", LoRaWANValidation.errorKey(cfg(rx2FreqHz = 859_000_000L)))
        assertEquals("adr", LoRaWANValidation.errorKey(cfg(adr = 2)))
        assertEquals("confirm", LoRaWANValidation.errorKey(cfg(confirm = 2)))
        assertEquals("confirmCnt", LoRaWANValidation.errorKey(cfg(confirmCnt = 0)))
        assertEquals("confirmCnt", LoRaWANValidation.errorKey(cfg(confirmCnt = 10)))
        assertEquals("power", LoRaWANValidation.errorKey(cfg(power = 8)))              // > EU868 powerMax
        assertEquals("dr", LoRaWANValidation.errorKey(cfg(dr = 7)))
        assertEquals("activation", LoRaWANValidation.errorKey(cfg(activation = 2)))
        assertEquals("deviceClass", LoRaWANValidation.errorKey(cfg(deviceClass = 1)))  // 只允许 0/2
        assertEquals("port", LoRaWANValidation.errorKey(cfg(port = 0)))
        assertEquals("port", LoRaWANValidation.errorKey(cfg(port = 224)))
        // chmask 越界位：EU868 仅 8 位，"0107" = ch0 开 + ch8-10 开（右字节=ch8-15）非法
        assertEquals("chmask", LoRaWANValidation.errorKey(cfg(chmask = ChmaskCodec.parseHex("0107")!!)))
        // chmask 全关
        assertEquals("chmask", LoRaWANValidation.errorKey(cfg(chmask = ByteArray(9))))
        // CN470 72 位全开 + 区域参数就位 → 合法（宽掩码透传场景）
        assertNull(
            LoRaWANValidation.errorKey(
                cfg(band = 6, rx2Dr = 3, rx2FreqHz = 505_300_000L, dr = 3,
                    chmask = ChmaskCodec.parseHex("FFFFFFFFFFFFFFFFFF")!!),
            ),
        )
        // US915 64 位：bit64-71 非法、bit63 合法（rx2Dr=8 为该区域 RX2 真实默认，已不触发前置分支）
        assertEquals(
            "chmask",
            LoRaWANValidation.errorKey(
                cfg(band = 90, rx2Dr = 8, rx2FreqHz = 923_300_000L, dr = 3,
                    chmask = hexToBytes("FFFFFFFFFFFFFFFF01")),
            ),
        )
        assertNull(
            LoRaWANValidation.errorKey(
                cfg(band = 90, rx2Dr = 8, rx2FreqHz = 923_300_000L, dr = 3,
                    chmask = ChmaskCodec.parseHex("FFFFFFFFFFFFFFFF")!!),
            ),
        )
    }

    @Test
    fun testChmaskDisplayByteOrderLeftIsLowChannel() {
        // 字节序直读（2026-11-14 用户真机裁决）：hex 串左起第一字节=ch0-7=载荷 chmask[0]=
        // 模组 AT+CHMASK 首字节。CN470 默认 0x00FF（ch0-7 开）→ "FF00"，旧 BE 数值形态废弃。
        assertEquals("0700", ChmaskCodec.toDisplay(ChmaskCodec.fromU16(0x0007)))   // EU868 默认 ch0-2
        assertEquals("FF00", ChmaskCodec.toDisplay(ChmaskCodec.fromU16(0x00FF)))   // CN470 默认 ch0-7
        assertEquals("00FF", ChmaskCodec.toDisplay(ChmaskCodec.fromU16(0xFF00)))   // ch8-15 开
        assertEquals("0000", ChmaskCodec.toDisplay(ByteArray(9)))
        assertEquals("FFFFFFFF", ChmaskCodec.toDisplay(ChmaskCodec.parseHex("FFFFFFFF")!!))        // ch0-31
        assertEquals("FFFFFFFFFFFFFFFFFF", ChmaskCodec.toDisplay(ChmaskCodec.parseHex("ffffffffffffffffff")!!))
        // parseHex 字节序：左字节落 chmask[0]（"0007" = ch8-10 开，8 信道区域即越界）
        assertEquals(0x07, ChmaskCodec.parseHex("0700")!![0].toInt() and 0xFF)
        assertEquals(0x07, ChmaskCodec.parseHex("0007")!![1].toInt() and 0xFF)
        assertTrue(ChmaskCodec.parseHex("0007")!!.contentEquals(ChmaskCodec.fromU16(0x0700)))
        // 奇数长度左补 0（"7"="07" → ch0-2 开）
        assertTrue(ChmaskCodec.parseHex("7")!!.contentEquals(ChmaskCodec.fromU16(7)))
        // 防线：非法字符 / 超长 / 空
        assertNull(ChmaskCodec.parseHex("1G"))
        assertNull(ChmaskCodec.parseHex("FFFFFFFFFFFFFFFFFFF"))   // 19 位
        assertNull(ChmaskCodec.parseHex(""))
        assertNull(ChmaskCodec.parseHex("  "))
    }

    @Test
    fun testPowerTableCoversEveryRegionPowerRange() {
        // 表长必须 ≥ powerMax+1，否则 UI dBm 下拉覆盖不到固件允许的索引范围
        LoRaWANRegions.all.forEach { r ->
            val list = LoRaWANPowerTable.dbmList(r.band)
            assertTrue(list.size >= r.powerMax + 1, "band ${r.band}: ${list.size} < ${r.powerMax + 1}")
            assertEquals(r.powerMax + 1, list.size)   // 精确等长（无多余档位）
        }
        // 代表值（固件注释同源；US915/AU915 为 RP002 标准表推定，真机核对项）
        assertEquals(17, LoRaWANPowerTable.dbmOf(6, 0))
        assertEquals(3, LoRaWANPowerTable.dbmOf(6, 7))
        assertEquals(16, LoRaWANPowerTable.dbmOf(86, 0))
        assertEquals(2, LoRaWANPowerTable.dbmOf(86, 7))
        assertEquals(22, LoRaWANPowerTable.dbmOf(85, 4))    // IN865 前 5 档同为 22
        assertEquals(20, LoRaWANPowerTable.dbmOf(85, 5))
        assertEquals(10, LoRaWANPowerTable.dbmOf(85, 10))
        assertEquals(30, LoRaWANPowerTable.dbmOf(90, 0))    // 推定表 30..2
        assertEquals(2, LoRaWANPowerTable.dbmOf(90, 14))
        assertEquals(14, LoRaWANPowerTable.dbmOf(92, 0))
        assertEquals(0, LoRaWANPowerTable.dbmOf(92, 7))
        // 越界防御
        assertNull(LoRaWANPowerTable.dbmOf(86, 8))
        assertNull(LoRaWANPowerTable.dbmOf(85, 11))
    }

    @Test
    fun testSr0ParseDocExampleFrame() {
        // 模组手册表 5-23/5-24 示例应答（用户 2026-11-14 截图）：帧头 Type04/Event0E/Len2C/RFU/
        // OpCode LE 0214/Result 00 + 40B STATUS0 + 帧尾 0D。STATUS0：05=JOIN 成功、上行 CH72/
        // DR5/16dBm、DevNonce LE=2、下行 TOA LE=0x0005F9CA、上行 TOA LE=0x0010475B（按大端读
        // 出 56 分钟量级 TOA，与"刚入网"场景不符 → 多字节字段小端定论）。
        val text =
            "FE 2F 40 8A 04 0E 2C 01 02 14 00 05 48 05 10 00 05 05 0A 00 00 00 00 00 00 00 00\n" +
                "00 00 00 00 00 00 00 00 02 00 00 CA F9 05 00 5B 47 10 00 00 00 00 00 0D"
        val st = LoRaWANSr0Parser.parse(text)!!
        assertEquals(5, st.commResult)
        assertEquals(72, st.uplinkCh)
        assertEquals(5, st.uplinkDr)
        assertEquals(16, st.uplinkPowerDbm)
        assertEquals(0, st.downlinkCh)
        assertEquals(5, st.rx1Dr)
        assertEquals(0, st.rx1DrOffset)
        assertEquals(5, st.rx2Dr)
        assertEquals(10, st.rxSlot)
        assertEquals(0, st.fport)
        assertEquals(0, st.gateways)
        assertEquals(0, st.snrDb)
        assertEquals(0, st.demodMarginDb)
        assertEquals(0, st.rssiDbm)
        assertEquals(0L, st.downFcnt)
        assertEquals(0L, st.upFcnt)
        assertEquals(0, st.joinCount)
        assertEquals(2, st.devNonce)
        assertEquals(391_370L, st.downToaUs)
        assertEquals(1_067_099L, st.upToaUs)
    }

    @Test
    fun testSr0ParseBare40BytesWithAtPrefixAndNegativeValues() {
        // 形态 2：无帧头纯 40B hex（AT 应答直给状态体）；"+SR0:" 前缀与逗号分隔不影响提取
        //（"SR0" 的 0 是独立 1 字符段被丢弃）。负值走补码：SNR int8、RSSI int16 LE（手册例
        // -87dBm → A9 FF）。
        val b = ByteArray(40)
        b[0] = 8                        // 发送成功，有下行
        b[5] = 0x21                     // RX1：DR_OFFSET=2、DR=1
        b[11] = 0xFF.toByte()           // SNR = -1 dB
        b[13] = 0xA9.toByte()           // RSSI 低字节
        b[14] = 0xFF.toByte()           // RSSI 高字节 → -87 dBm
        b[19] = 2                       // 上行 FCNT u32 LE = 0x00010002（字节 02 00 01 00）
        b[21] = 1
        b[27] = 0x12.toByte()           // TOA 溢出：下行 1 次、上行 2 次
        val text = "+SR0:" + b.joinToString(",") { "%02X".format(it) }
        val st = LoRaWANSr0Parser.parse(text)!!
        assertEquals(8, st.commResult)
        assertEquals(1, st.rx1Dr)
        assertEquals(2, st.rx1DrOffset)
        assertEquals(-1, st.snrDb)
        assertEquals(-87, st.rssiDbm)
        assertEquals(0x1_0002L, st.upFcnt)
        assertEquals(1, st.toaOverflowDown)
        assertEquals(2, st.toaOverflowUp)
    }

    @Test
    fun testSr0ParseRejectsUnrecognized() {
        assertNull(LoRaWANSr0Parser.parse(""))
        assertNull(LoRaWANSr0Parser.parse("SR0"))       // 仅 "0" 单字符段，不构成字节
        assertNull(LoRaWANSr0Parser.parse("ERROR"))
        assertNull(LoRaWANSr0Parser.parse("05 48"))     // 2 字节：既非帧头也非 40B
    }

    @Test
    fun testSr0ParseRealDeviceResponse() {
        // 2026-11-14 真机实测（CN470 设备）："+SR0:" + 40B hex + "OK" —— 纯 40B 形态（形态 2）。
        // 小端结论二次验证：JOIN 累计=12、DevNonce=946（0x03B2）、上行 TOA=16926µs（单次上行
        // 16.9ms 量级）；16dBm 与 CN470 功率上限 17dBm 吻合。
        val text = "+SR0: 00 02 03 10 00 04 00 0F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 " +
            "0C 00 B2 03 00 00 00 00 00 1E 42 00 00 00 00 00 00\r\nOK"
        val st = LoRaWANSr0Parser.parse(text)!!
        assertEquals(0, st.commResult)       // 无数据操作
        assertEquals(2, st.uplinkCh)
        assertEquals(3, st.uplinkDr)
        assertEquals(16, st.uplinkPowerDbm)
        assertEquals(0, st.downlinkCh)
        assertEquals(4, st.rx1Dr)
        assertEquals(0, st.rx1DrOffset)
        assertEquals(0, st.rx2Dr)
        assertEquals(15, st.rxSlot)
        assertEquals(12, st.joinCount)
        assertEquals(946, st.devNonce)
        assertEquals(16_926L, st.upToaUs)
        assertEquals(0L, st.downToaUs)
    }
}
