package com.hkt.ble.bletools.model

import com.hkt.ble.bletools.core.ble.ConnectFailure
import com.hkt.ble.bletools.core.ble.DiscoveredDevice
import com.hkt.ble.bletools.core.ble.MockCentral
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.advanceTimeBy
import kotlinx.coroutines.test.runCurrent
import kotlinx.coroutines.test.runTest
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

/// ConnectModel 胶水层测试（M6.1 评审 P2-9）：outcome 分类 / retry 复位 / 超时失败。
/// iOS 同为薄壳无直测——此处补齐 Kotlin 侧。
@OptIn(ExperimentalCoroutinesApi::class)
class ConnectModelTest {
    private val target = DiscoveredDevice("SVC100_0D137C", "id-1", -55)

    @Test
    fun testSuccessOutcomeAdvancesToPhase3() = runTest {
        val mock = MockCentral(backgroundScope)
        mock.connectScript = MockCentral.ConnectScript.Success(0)
        val model = ConnectModel(target, mock, backgroundScope)
        var finished: ConnectModel.Outcome? = null
        model.onFinished = { finished = it }

        model.start()
        runCurrent()
        assertEquals(ConnectModel.Outcome.CONNECTED, model.outcome.value)
        assertEquals(3, model.phaseIndex.value)
        assertEquals(ConnectModel.Outcome.CONNECTED, finished)
    }

    @Test
    fun testCancelYieldsCancelledOutcome() = runTest {
        val mock = MockCentral(backgroundScope)
        mock.connectScript = MockCentral.ConnectScript.Success(delayMs = 500)
        val model = ConnectModel(target, mock, backgroundScope)
        model.start()
        runCurrent()
        model.cancel()
        runCurrent()
        assertEquals(ConnectModel.Outcome.CANCELLED, model.outcome.value)
        assertEquals(ConnectFailure.CANCELLED, model.failure.value)
    }

    @Test
    fun testRetryResetsOutcomeThenSucceeds() = runTest {
        val mock = MockCentral(backgroundScope)
        mock.connectScript = MockCentral.ConnectScript.Never
        val model = ConnectModel(target, mock, backgroundScope)
        model.start()
        runCurrent()
        model.cancel()
        runCurrent()
        assertEquals(ConnectModel.Outcome.CANCELLED, model.outcome.value)

        // retry：outcome 必须先复位，成功后置 CONNECTED
        mock.connectScript = MockCentral.ConnectScript.Success(0)
        model.retry()
        runCurrent()
        assertEquals(ConnectModel.Outcome.CONNECTED, model.outcome.value)
    }

    @Test
    fun testLinkTimeoutFails() = runTest {
        val mock = MockCentral(backgroundScope)
        mock.connectScript = MockCentral.ConnectScript.Never
        val model = ConnectModel(target, mock, backgroundScope)
        model.start()
        // 默认预算：连接阶段 10s
        advanceTimeBy(10_000)
        runCurrent()
        assertEquals(ConnectModel.Outcome.FAILED, model.outcome.value)
        assertEquals(ConnectFailure.TIMEOUT_LINK, model.failure.value)
    }
}
