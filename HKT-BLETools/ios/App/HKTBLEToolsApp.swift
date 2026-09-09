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
            scanModel = ScanModel(port: mock, autoStartOnReady: true)
            let responder = DemoResponder()
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
