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
    /// SP-1：信号强度阈值（默认 -80 dBm，设置页可调并持久化）。
    var rssiThreshold: Int = -80

    /// R-32：返回首页时的驻留会话（nil=无会话）。
    var residentDevice: ResidentDevice?

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
        guard isReady, !isScanning else { return }
        isScanning = true
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
                if self.isScanning { self.devices = devices }
            }
        }
    }

    func stopScan() {
        isScanning = false
        central.stopScan()
    }

    // MARK: - R-2 目标设备定位（2026-09-07 裁决选 B）

    func beginLocateInput() { locatePhase = .input }

    /// SP-2 输入校验。
    var locateInput = "0095690" {   // 预填厂商前缀（Android DEFAULT_DEV_EUI_PREFIX 同源）
        didSet {
            let filtered = locateInput.uppercased().filter { $0.isHexDigit }.prefix(16)
            if filtered != locateInput { locateInput = String(filtered) }
        }
    }
    var locateInputValid: Bool { locateInput.count == 16 }

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
}
