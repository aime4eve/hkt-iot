package com.hkt.ble.bletools.core.ble

/**
 * 已连接会话的收发链路（DeviceSession 只面向此接口；真实现 = SystemCentral 链路，测试/演示 = MockLink）。
 * 契约：onReceive/onDisconnected 在主线程投递（真实现负责线程切换）。
 */
interface PeripheralLink {
    /** 发送一条协议帧（写 Write 特征）。 */
    fun send(frame: ByteArray)

    /** 收到一条协议帧（Indicate 通知）。 */
    var onReceive: ((ByteArray) -> Unit)?

    /** 对端断开（R-6 自动重连 / R-31 语义由上层处理）。 */
    var onDisconnected: (() -> Unit)?
}

/** 测试/预览用假链路：responder 按请求帧返回响应帧；记录已发帧供断言。 */
class MockLink : PeripheralLink {
    private val sent = ArrayList<ByteArray>()

    var responder: ((ByteArray) -> ByteArray?)? = null
    override var onReceive: ((ByteArray) -> Unit)? = null
    override var onDisconnected: (() -> Unit)? = null

    override fun send(frame: ByteArray) {
        synchronized(sent) { sent.add(frame) }
        responder?.invoke(frame)?.let { response -> onReceive?.invoke(response) }
    }

    /** 场景注入：模拟对端断开。 */
    fun simulateDisconnect() {
        onDisconnected?.invoke()
    }

    val framesSent: Int get() = synchronized(sent) { sent.size }

    fun sentFrameAt(index: Int): ByteArray = synchronized(sent) { sent[index] }
}
