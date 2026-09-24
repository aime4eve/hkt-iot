package com.hkt.ble.bletools.core.ble

/**
 * 蓝牙传输端口（SD 架构测试缝）：真蓝牙（SystemCentral）与可编程假蓝牙源（MockCentral）共用同一接口。
 * 契约：所有回调在主线程投递（对应 iOS「回调主线程投递」）。
 */
interface BluetoothPort {
    /**
     * 激活：创建系统管理器并持续回报能力状态（不扫描）。App 启动即调用——
     * 真机上这一步触发系统蓝牙权限弹窗（R-24）。
     */
    fun activate(onUpdate: (BLEAvailability) -> Unit)

    /** 开始扫描：就绪后自动发现；入列规则 = [ScanOptions]（阈值+前缀+无名称排除）。 */
    fun startScan(options: ScanOptions, onUpdate: (BLEAvailability, List<DiscoveredDevice>) -> Unit)

    /** 停止扫描（能力状态回报继续，R-32 驻留健康检测依赖）。 */
    fun stopScan()

    /**
     * 连接三阶段（P-02/SP-4）：事件为"完成语义"，驱动 ConnectOrchestrator 推进；
     * 超时由编排器按 ConnectBudget 判定，与传输实现无关。
     */
    fun connect(device: DiscoveredDevice, events: (ConnectEvent) -> Unit)

    /** 取消底层连接尝试（编排器取消后由 App 层调用；实现须停止推进事件）。 */
    fun cancelConnect()

    /** R-31：断开当前已连接设备（GATT disconnect）。 */
    fun disconnectDevice()

    /** 连接建立后的收发链路（设备会话轮询/发命令用）；未连接或目标不符返回 null。 */
    fun makeLink(forDevice: DiscoveredDevice): PeripheralLink?
}
