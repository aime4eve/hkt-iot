import CoreBLE
import SwiftUI

@main
struct HKTBLEToolsApp: App {
    @State private var scanModel: ScanModel

    init() {
        if ProcessInfo.processInfo.arguments.contains("-mockble") {
            // 演示模式（模拟器无蓝牙硬件）：假蓝牙源 + 脚本设备 + 仿真应答 + 自动扫描
            let mock = MockCentral()
            mock.setScriptedDevices([
                .init(name: "MPS100 9C01", identifier: UUID(), rssi: -63),
                .init(name: "UDS100 3F2A", identifier: UUID(), rssi: -58),
                .init(name: "SVC100 B4D2", identifier: UUID(), rssi: -70),
                .init(name: "EPS100 77AA", identifier: UUID(), rssi: -91),
            ])
            // MOCK_CONN_DELAY 可调慢连接脚本（秒），便于截取连接覆盖层各阶段；
            // MOCK_AVAILABILITY 可注入能力状态（ready/poweredOff/denied/unsupported），便于验收 P-08 各态
            let delay = ProcessInfo.processInfo.environment["MOCK_CONN_DELAY"]
                .flatMap(Double.init) ?? 0.3
            mock.connectScript = .success(delay: delay)
            if let injected = ProcessInfo.processInfo.environment["MOCK_AVAILABILITY"] {
                switch injected {
                case "initializing": break   // 保持初始态（P-08 ask 门）
                case "poweredOff": mock.setAvailability(.poweredOff)
                case "denied": mock.setAvailability(.denied)
                case "unsupported": mock.setAvailability(.unsupported)
                default: mock.setAvailability(.ready)
                }
            } else {
                mock.setAvailability(.ready)
            }
            // -demo-flow <设备前缀>：扫描命中即自动停扫直连进详情；
            // 无 -demo-flow 时 7 秒自动停扫（原型 toggleScan 行为 1:1）
            let arguments = ProcessInfo.processInfo.arguments
            let demoFlowPrefix: String?
            if let flowIndex = arguments.firstIndex(of: "-demo-flow"),
               arguments.count > flowIndex + 1 {
                demoFlowPrefix = arguments[flowIndex + 1]
            } else {
                demoFlowPrefix = nil
            }
            scanModel = ScanModel(port: mock, autoStartOnReady: true,
                                  demoAutoStopAfter: demoFlowPrefix == nil ? 7 : nil,
                                  demoAutoConnectPrefix: demoFlowPrefix)
            let responder = DemoResponder()
            // MOCK_CFG_ACK=0：模拟固件静默拒绝（不回 ACK），验收配置页失败横幅
            responder.configAcks = ProcessInfo.processInfo.environment["MOCK_CFG_ACK"] != "0"
            mock.responder = { [weak scanModel] frame in
                let family = MainActor.assumeIsolated { scanModel?.activeSession?.family }
                return responder.respond(to: frame, family: family)
            }
        } else {
            scanModel = ScanModel()
        }
    }

    var body: some Scene {
        WindowGroup {
            RootView()
                .environment(scanModel)
                .environment(LanguageStore.shared)
        }
    }
}
