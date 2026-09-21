package com.hkt.ble.bletools.core.ble

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNull
import kotlin.test.assertTrue
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.advanceTimeBy
import kotlinx.coroutines.test.runCurrent
import kotlinx.coroutines.test.runTest

/// 扫描入列规则与编排器/假蓝牙源行为。iOS 对照：ScanRuleTests(8) + ConnectOrchestratorTests(5) + MockConnectTests(4)。
@OptIn(ExperimentalCoroutinesApi::class)
class ScanRuleTests {
    // MARK: 名称前缀（SP-1）

    @Test
    fun testPrefixExtraction() {
        assertEquals("UDS", DiscoveredDevice.prefixOf("UDS100 3F2A"))
        assertEquals("SVC", DiscoveredDevice.prefixOf("SVC100_0D137C"))
        assertEquals("EPS", DiscoveredDevice.prefixOf("  eps100 "))   // trim + 大写
        assertNull(DiscoveredDevice.prefixOf("ab"))
        assertNull(DiscoveredDevice.prefixOf(null))
        assertNull(DiscoveredDevice.prefixOf(""))
    }

    @Test
    fun testSupportedPrefixes() {
        assertEquals(setOf("MPS", "SVC", "UDS", "EPS"), DiscoveredDevice.supportedPrefixes)
    }

    // MARK: 入列规则（R-1：RSSI 阈值 && 前缀命中；无名称天然排除）

    @Test
    fun testIsListable() {
        val options = ScanOptions()
        assertTrue(options.isListable("UDS100 3F2A", -70))
        assertFalse(options.isListable("UDS100 3F2A", -90))    // 低于阈值
        assertFalse(options.isListable("XYZ-9", -70))          // 前缀不在集合
        assertFalse(options.isListable(null, -70))             // 无名称
        assertFalse(options.isListable("ab", -70))             // 不足 3 字符
        // 目标定位场景：后缀匹配忽略前缀/阈值过滤的语义在 ScanModel（App 层，M5 接线）
    }

    @Test
    fun testCustomAllowedPrefixes() {
        val options = ScanOptions(allowedPrefixes = setOf("UDS"), rssiThreshold = -60)
        assertTrue(options.isListable("UDS100", -60))
        assertFalse(options.isListable("SVC100", -60))
        assertFalse(options.isListable("UDS100", -61))   // 边界：阈值含等号
    }

    // MARK: MockCentral 过滤/去重/排序

    @Test
    fun testMockCentralFilterSortDedupe() = runTest {
        val central = MockCentral(this)
        var listed: List<DiscoveredDevice> = emptyList()
        central.startScan(ScanOptions(rssiThreshold = -80)) { _, devices -> listed = devices }

        central.discover(DiscoveredDevice("SVC100_0D137C", "id-1", -55))
        central.discover(DiscoveredDevice("UDS100_3F2A", "id-2", -70))
        central.discover(DiscoveredDevice("EPS100_0288CF", "id-3", -90))   // 低于阈值不入列
        central.discover(DiscoveredDevice("MPS100_9C01", "id-4", -62))
        central.discover(DiscoveredDevice("SVC100_0D137C", "id-1", -58))  /// 同 id 更新（去重）

        assertEquals(listOf("id-1", "id-4", "id-2"), listed.map { it.identifier }, "RSSI 降序 + 阈值过滤 + 同 id 去重")
        assertEquals(-58, listed.first().rssi)
    }

    @Test
    fun testMockCentralClearDevices() = runTest {
        val central = MockCentral(this)
        var listed: List<DiscoveredDevice>? = null
        central.startScan(ScanOptions()) { _, devices -> listed = devices }
        central.discover(DiscoveredDevice("SVC100", "id-1", -55))
        central.clearDevices()
        assertEquals(emptyList(), listed)
    }

    // MARK: 连接编排器（P-02/SP-4）

    @Test
    fun testOrchestratorAdvancesThreePhases() = runTest {
        val orchestrator = ConnectOrchestrator(this)
        var finishFailure: ConnectFailure? = ConnectFailure.CONNECTION_LOST
        orchestrator.onFinish = { finishFailure = it }
        orchestrator.begin()
        assertEquals(ConnectPhase.LINK, orchestrator.phase)
        assertTrue(orchestrator.isActive)
        orchestrator.advance()
        assertEquals(ConnectPhase.SERVICES, orchestrator.phase)
        orchestrator.advance()
        assertEquals(ConnectPhase.SUBSCRIBING, orchestrator.phase)
        orchestrator.advance()
        assertTrue(orchestrator.isConnected)
        assertNull(finishFailure)
        assertFalse(orchestrator.isActive)
        // 终态后 advance 不再推进
        orchestrator.advance()
        assertNull(orchestrator.phase)
    }

    @Test
    fun testOrchestratorPhaseTimeout() = runTest {
        val orchestrator = ConnectOrchestrator(this)
        var finishFailure: ConnectFailure? = null
        orchestrator.onFinish = { finishFailure = it }
        orchestrator.begin(ConnectOrchestrator.Budget(linkMs = 100, servicesMs = 50, subscribingMs = 50))
        advanceTimeBy(100)
        runCurrent()
        assertEquals(ConnectFailure.TIMEOUT_LINK, finishFailure)
        assertFalse(orchestrator.isActive)
    }

    @Test
    fun testOrchestratorCancelAndAbort() = runTest {
        val orchestrator = ConnectOrchestrator(this)
        var finishFailure: ConnectFailure? = null
        orchestrator.onFinish = { finishFailure = it }
        orchestrator.begin()
        orchestrator.cancel()
        assertEquals(ConnectFailure.CANCELLED, finishFailure)
        assertTrue(orchestrator.isCancelled)

        // 终态后 abort 被忽略
        orchestrator.abort(ConnectFailure.SERVICE_MISSING)
        assertEquals(ConnectFailure.CANCELLED, finishFailure)
    }

    @Test
    fun testOrchestratorAbortServiceMissing() = runTest {
        val orchestrator = ConnectOrchestrator(this)
        var finishFailure: ConnectFailure? = null
        orchestrator.onFinish = { finishFailure = it }
        orchestrator.begin()
        orchestrator.advance()
        orchestrator.abort(ConnectFailure.SERVICE_MISSING)
        assertEquals(ConnectFailure.SERVICE_MISSING, finishFailure)
    }

    @Test
    fun testOrchestratorRestartResetsState() = runTest {
        val orchestrator = ConnectOrchestrator(this)
        orchestrator.begin()
        orchestrator.cancel()
        orchestrator.begin()
        assertEquals(ConnectPhase.LINK, orchestrator.phase)
        assertEquals(null, orchestrator.failure)
        assertFalse(orchestrator.isCancelled)
        assertTrue(orchestrator.isActive)
    }

    // MARK: MockCentral 连接脚本（MockConnect 同构）

    @Test
    fun testMockConnectSuccessScript() = runTest {
        val central = MockCentral(this)
        val device = DiscoveredDevice("SVC100_0D137C", "id-1", -55)
        val events = ArrayList<ConnectEvent>()
        central.connectScript = MockCentral.ConnectScript.Success(0)
        central.connect(device) { events.add(it) }
        runCurrent()
        assertEquals(
            listOf(ConnectEvent.LinkEstablished, ConnectEvent.ServicesDiscovered, ConnectEvent.NotificationsEnabled),
            events,
        )
        assertTrue(central.makeLink(device) != null)
        assertTrue(central.makeLink(DiscoveredDevice("other", "id-9", -50)) == null)
    }

    @Test
    fun testMockConnectNeverScript() = runTest {
        val central = MockCentral(this)
        val events = ArrayList<ConnectEvent>()
        central.connectScript = MockCentral.ConnectScript.Never
        central.connect(DiscoveredDevice("SVC100", "id-1", -55)) { events.add(it) }
        advanceTimeBy(60_000)
        runCurrent()
        assertEquals(emptyList(), events, "Never 脚本永不推进（弱信号失败分支）")
        assertNull(central.makeLink(DiscoveredDevice("SVC100", "id-1", -55)))
    }

    @Test
    fun testMockConnectDelayedScriptCancellable() = runTest {
        val central = MockCentral(this)
        val events = ArrayList<ConnectEvent>()
        central.connectScript = MockCentral.ConnectScript.Success(delayMs = 500)
        central.connect(DiscoveredDevice("SVC100", "id-1", -55)) { events.add(it) }
        advanceTimeBy(100)
        runCurrent()
        assertEquals(emptyList(), events)
        central.cancelConnect()
        advanceTimeBy(1_000)
        runCurrent()
        assertEquals(emptyList(), events, "取消后不得再推进事件")
    }

    @Test
    fun testMockCentralRespondsAndInjects() = runTest {
        val central = MockCentral(this)
        val sessionLink = central   // MockCentral 兼任 PeripheralLink
        var received: ByteArray? = null
        sessionLink.onReceive = { received = it }
        central.responder = { frame -> if (frame.contentEquals(byteArrayOf(0x01))) byteArrayOf(0x02) else null }
        sessionLink.send(byteArrayOf(0x01))
        assertEquals(0x02.toByte(), received?.get(0))
        central.inject(byteArrayOf(0x03))
        assertEquals(0x03.toByte(), received?.get(0))
        central.simulateDisconnect()
        // onDisconnected 未挂不崩（空安全）
    }
}
