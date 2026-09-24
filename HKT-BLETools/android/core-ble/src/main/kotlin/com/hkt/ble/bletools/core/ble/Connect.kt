package com.hkt.ble.bletools.core.ble

import java.util.UUID

/**
 * HKT 透明桥服务与特征 UUID（Android BLEUtils.kt 核实，固件模组侧配置）。
 */
object HKTProfile {
    val serviceUUID: UUID = UUID.fromString("0783B03E-8535-B5A0-7140-A304D2495CB7")

    /** Indicate 特征：设备 → App（状态/响应） */
    val indicateUUID: UUID = UUID.fromString("0783B03E-8535-B5A0-7140-A304D2495CB8")

    /** Write 特征：App → 设备（命令帧） */
    val writeUUID: UUID = UUID.fromString("0783B03E-8535-B5A0-7140-A304D2495CBA")
}

/**
 * 连接进展事件（完成语义：每个事件 = 对应 ConnectPhase 完成，编排器据此推进）。
 */
sealed class ConnectEvent {
    /** 链路建立（didConnect） */
    data object LinkEstablished : ConnectEvent()

    /** HKT 服务与特征发现完成 */
    data object ServicesDiscovered : ConnectEvent()

    /** Indicate 订阅完成（链路就绪，可收发协议帧） */
    data object NotificationsEnabled : ConnectEvent()

    /** 连接过程失败（原因见 [ConnectFailure]） */
    data class Failed(val failure: ConnectFailure) : ConnectEvent()
}

/** 连接失败原因（R-G4：带阶段定位，禁止统一"失败"）。 */
enum class ConnectFailure {
    /** SP-4 阶段预算超时（连接 10s / 发现 5s / 订阅 5s） */
    TIMEOUT_LINK, TIMEOUT_SERVICES, TIMEOUT_SUBSCRIBING,

    /** HKT 服务缺失（发现服务阶段） */
    SERVICE_MISSING,

    /** 链路中断 */
    CONNECTION_LOST,

    /** 用户取消 */
    CANCELLED,
}
