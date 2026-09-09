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

    private let central = SystemCentral()

    init() {
        central.onUpdate = { [weak self] availability, devices in
            MainActor.assumeIsolated {
                guard let self else { return }
                self.availability = availability
                // 驻留/列表冻结规则（R-32）：列表仅在扫描进行中刷新。
                if self.isScanning { self.devices = devices }
            }
        }
    }

    var isReady: Bool { availability.isUsable }

    func startScan() {
        guard isReady, !isScanning else { return }
        isScanning = true
        central.start(options: .init(allowedPrefixes: allowedPrefixes, rssiThreshold: rssiThreshold))
    }

    func stopScan() {
        isScanning = false
    }
}
