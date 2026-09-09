import CoreBLE
import CoreDevice
import SwiftUI

/// 会话驻留期可见的设备卡片模型（R-32）：已连接设备置顶带徽章，点击直接回详情。
struct ResidentDevice: Equatable, Sendable {
    var name: String
    var identifier: UUID
}

/// P-01 扫描页模型：能力状态 + 去重发现列表 + 手动启停扫描（R-1/SP-1）。
/// 驻留会话（R-32）由 DeviceSession 层（后续里程碑）注入 residentDevice。
@MainActor
@Observable
final class ScanModel {
    private(set) var availability: BLEAvailability = .initializing
    private(set) var devices: [DiscoveredDevice] = []
    private(set) var isScanning = false

    /// SP-1：名称前缀过滤（MPS/SVC/UDS/EPS，设置页可调并持久化）。
    var allowedPrefixes: Set<String> = DiscoveredDevice.supportedPrefixes

    /// P-07 扫描过滤：勾选/取消某前缀。
    func togglePrefix(_ prefix: String) {
        if allowedPrefixes.contains(prefix) {
            allowedPrefixes.remove(prefix)
        } else {
            allowedPrefixes.insert(prefix)
        }
        // 已在列的设备按新规则复筛
        if isScanning { devices = devices.filter { DiscoveredDevice.prefix(of: $0.name).map(allowedPrefixes.contains) ?? false } }
    }
    /// SP-1：信号强度阈值（默认 -80 dBm，设置页可调并持久化）。
    var rssiThreshold: Int = -80

    /// R-32：返回首页时的驻留会话（nil=无会话）。
    var residentDevice: ResidentDevice?

    /// 连接成功后请求打开详情页（覆盖层内触发，扫描页 onChange 响应）。
    var requestShowDetail = false

    // R-2 目标设备定位
    public enum LocatePhase: Equatable { case input, finding, notFound }
    private(set) var locatePhase: LocatePhase?
    private(set) var locateSuffix: String?
    private(set) var locateHitDevice: DiscoveredDevice?
    /// 定位命中回调（RootView 据此弹出连接覆盖层）。
    public var onLocateHit: ((DiscoveredDevice) -> Void)?
    private var locateTimeoutTask: Task<Void, Never>?
    private var savedPrefixes: Set<String>?

    /// 演示模式（-mockble 启动参数）：能力就绪后自动开始扫描。
    var autoStartOnReady = false
    private var autoStarted = false
    /// 本次扫描已记日志的设备（防重复）
    private var loggedDiscoveries = Set<UUID>()
    /// 演示自动导航（-demo-flow）：扫描发现该前缀设备即停扫直连进详情
    var demoAutoConnectPrefix: String?

    private let central: any BluetoothPort

    init(port: any BluetoothPort = SystemCentral(), autoStartOnReady: Bool = false) {
        self.autoStartOnReady = autoStartOnReady
        central = port
        // 启动即激活（真机：触发系统蓝牙权限弹窗，R-24）；就绪后按需自动扫描（演示模式）
        central.activate { [weak self] availability in
            MainActor.assumeIsolated {
                guard let self else { return }
                self.availability = availability
                if availability.isUsable, self.autoStartOnReady, !self.autoStarted, !self.isScanning {
                    self.autoStarted = true
                    self.startScan()
                }
            }
        }
    }

    var isReady: Bool { availability.isUsable }

    func startScan() {
        // R-32 重扫健康检测（原型 toggleScan）：活会话最后成功轮询距今 >5s = 无蓝牙信号
        //（连接态设备已停止广播，不能以扫描可见性判活，只看轮询心跳）→ 释放并原身份重连
        if let session = activeSession, !session.linkLost,
           let stale = session.secondsSinceLastResponse, stale > 5 {
            activeSession?.stop()
            activeSession = nil
            central.disconnectDevice()
            reconnectResident()
            return
        }
        guard isReady, !isScanning else { return }
        isScanning = true
        loggedDiscoveries = []
        LogStore.shared.info(AppLocale.isZh ? "扫描启动" : "Scan started")
        central.startScan(options: .init(allowedPrefixes: allowedPrefixes, rssiThreshold: rssiThreshold)) { [weak self] availability, devices in
            MainActor.assumeIsolated {
                guard let self else { return }
                self.availability = availability
                // R-2 定位模式：后缀命中即停扫直连（优先于常规列表）
                if self.locatePhase == .finding, let suffix = self.locateSuffix,
                   let hit = devices.first(where: { $0.name.uppercased().contains(suffix) }) {
                    self.locateHitDevice = hit
                    self.locatePhase = nil
                    self.locateSuffix = nil
                    self.locateTimeoutTask?.cancel()
                    self.stopScan()
                    if let saved = self.savedPrefixes { self.allowedPrefixes = saved; self.savedPrefixes = nil }
                    self.onLocateHit?(hit)
                    return
                }
                // 驻留/列表冻结规则（R-32）：列表仅在扫描进行中刷新。
                if self.isScanning {
                    for device in devices where !self.loggedDiscoveries.contains(device.identifier) {
                        self.loggedDiscoveries.insert(device.identifier)
                        LogStore.shared.info(AppLocale.isZh ? "发现设备 \(device.name) \(device.rssi)dBm"
                                                            : "Discovered \(device.name) \(device.rssi)dBm")
                    }
                    self.devices = devices
                    // 演示自动导航：命中前缀即停扫直连进详情
                    if let prefix = self.demoAutoConnectPrefix,
                       let hit = devices.first(where: { $0.name.hasPrefix(prefix) }) {
                        self.demoAutoConnectPrefix = nil
                        self.stopScan()
                        self.requestShowDetail = true
                        _ = self.makeAndStartSession(for: hit)
                    }
                }
            }
        }
    }

    func stopScan() {
        guard isScanning else { return }
        isScanning = false
        central.stopScan()
        LogStore.shared.info(AppLocale.isZh ? "扫描停止" : "Scan stopped")
    }

    // MARK: - R-2 目标设备定位（2026-09-07 裁决选 B）

    func beginLocateInput() { locatePhase = .input }

    func retryLocate() {
        guard locateSuffix != nil else { return }
        locatePhase = .finding
        startScan()
        armLocateTimeout()
    }

    func beginCameraLocate() { /* 相机会话随相机里程碑接入 */ }

    private func armLocateTimeout() {
        locateTimeoutTask?.cancel()
        locateTimeoutTask = Task { [weak self] in
            try? await Task.sleep(for: .seconds(30))   // 3 轮 × 10s（与扫描节奏一致）
            guard let self, !Task.isCancelled, self.locatePhase == .finding else { return }
            self.locatePhase = .notFound
            self.stopScan()
        }
    }

    /// SP-2：校验 16 位 hex → 取后 6 位匹配广播名 → 目标模式忽略前缀过滤。
    func startLocate(devEUI: String) {
        let cleaned = devEUI.uppercased()
        guard cleaned.count == 16, cleaned.allSatisfy({ $0.isHexDigit }) else { return }
        locateSuffix = String(cleaned.suffix(6))
        locatePhase = .finding
        savedPrefixes = allowedPrefixes
        allowedPrefixes = DiscoveredDevice.supportedPrefixes   // 目标模式忽略前缀过滤
        startScan()
        armLocateTimeout()
    }

    func cancelLocate() {
        locateTimeoutTask?.cancel()
        if locatePhase == .finding { stopScan() }
        restoreAfterLocate()
    }

    private func restoreAfterLocate() {
        locateSuffix = nil
        if let saved = savedPrefixes { allowedPrefixes = saved; savedPrefixes = nil }
    }

    /// 为指定设备创建连接过程模型（P-02；端口与扫描同源）。
    func connector(for device: DiscoveredDevice) -> ConnectModel {
        ConnectModel(target: device, port: central)
    }

    /// 连接成功后的设备会话（家族判定 + 已连接链路）。
    func makeSession(for device: DiscoveredDevice) -> DeviceSession? {
        guard let family = DeviceRegistry.matchBroadcast(device.name)?.family else { return nil }
        guard let link = central.makeLink(for: device) else { return nil }
        return DeviceSession(family: family, deviceName: device.name, link: link)
    }

    /// R-32/R-31：当前活动会话（连接成功即建，断开即清）。
    public private(set) var activeSession: DeviceSession?

    /// 连接成功后建会话并启动 1s 轮询。
    @discardableResult
    func makeAndStartSession(for device: DiscoveredDevice) -> DeviceSession? {
        guard activeSession == nil, let session = makeSession(for: device) else { return activeSession }
        activeSession = session
        session.start()
        return session
    }

    /// R-31 断开连接：停轮询 + GATT 断开 + 清驻留（设备进最近设备，原型 releaseSession 语义）。
    func disconnectActive() {
        if let resident = residentDevice { lastSession = resident }
        activeSession?.stop()
        activeSession = nil
        central.disconnectDevice()
        residentDevice = nil
        LogStore.shared.info(AppLocale.isZh ? "手动断开（预期断开，不自动重连）" : "Manual disconnect (expected, no auto-reconnect)")
    }

    /// P-01 断线态「重新连接」：驻留设备原身份重建会话（系统侧 peripheral 通常仍在缓存，
    /// 不在扫描列表也能按 identifier 直连；完整 R-6 自动重连状态机另行接入）。
    @discardableResult
    func reconnectResident() -> DeviceSession? {
        guard activeSession == nil, let resident = residentDevice else { return nil }
        let device = devices.first { $0.identifier == resident.identifier }
            ?? DiscoveredDevice(name: resident.name, identifier: resident.identifier, rssi: Int.min)
        return makeAndStartSession(for: device)
    }

    // MARK: - P-01 首页卡片点击语义（R-31/R-32，原型 deviceCardClick）

    /// R-32 切换确认框待连接目标（nil = 无待确认）。
    private(set) var pendingSwitch: DiscoveredDevice?

    /// 最近一次释放的会话（R-31：断开后设备进"最近设备"，空列表时显示）。
    private(set) var lastSession: ResidentDevice?

    enum CardTapOutcome {
        case showDetail                                  // 验活通过：同一设备直接回详情
        case confirmSwitch(target: DiscoveredDevice)     // 其他设备 → 切换确认框
        case connect(ConnectModel)                       // 标准连接
    }

    func cardTapOutcome(for device: DiscoveredDevice) -> CardTapOutcome {
        if let session = activeSession, !session.linkLost, let resident = residentDevice {
            if device.identifier == resident.identifier { return .showDetail }
            return .confirmSwitch(target: device)
        }
        if activeSession != nil { disconnectActive() }   // 失联兜底：原子释放旧会话
        stopScan()                                        // 安卓同款：连接前先停扫
        return .connect(connector(for: device))
    }

    /// R-32 确认切换：原子释放当前会话（预期断开）→ 标准连接新设备。
    func confirmSwitch() -> ConnectModel? {
        guard let target = pendingSwitch else { return nil }
        pendingSwitch = nil
        disconnectActive()
        stopScan()
        return connector(for: target)
    }

    func cancelSwitch() { pendingSwitch = nil }

    func requestSwitch(to device: DiscoveredDevice) { pendingSwitch = device }

    /// 最近设备一键连接（空列表时的 recent 卡）。
    func connectorForLastSession() -> ConnectModel? {
        guard let last = lastSession else { return nil }
        let device = devices.first { $0.identifier == last.identifier }
            ?? DiscoveredDevice(name: last.name, identifier: last.identifier, rssi: Int.min)
        stopScan()
        return connector(for: device)
    }
}
