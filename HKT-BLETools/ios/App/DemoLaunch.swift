import Foundation

/// 演示自动导航参数解析（-demo-flow <设备前缀> -demo-page <目标页>）。
/// ScanModel.demoAutoConnectPrefix 消费设备前缀；各视图按 page 自动 push。
enum DemoLaunch {
    /// -demo-page 的目标页值（locate / locate-finding / config / tasks / ota / ota-run …）。
    static var page: String? {
        let arguments = ProcessInfo.processInfo.arguments
        guard let index = arguments.firstIndex(of: "-demo-page"),
              arguments.count > index + 1 else { return nil }
        return arguments[index + 1]
    }

    static func isPage(_ name: String) -> Bool {
        page == name
    }
}
