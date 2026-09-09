import SwiftUI

/// 隐私说明页（P_privacy）—— 1:1 克隆冻结原型（规格卡 `docs/ios/design/ui-spec/P-privacy.md`）。
/// 正文=原型 PRIVACY 对象逐字移植（ZH/EN 各 11 节）；随语言切换。
struct PrivacyView: View {
    @Environment(LanguageStore.self) private var langStore
    @Environment(\.dismiss) private var dismiss

    private var zh: Bool { langStore.isZh }

    var body: some View {
        VStack(spacing: 0) {
            NavbarHeader(title: zh ? "隐私说明" : "Privacy",
                         backText: zh ? "‹ 返回" : "‹ Back",
                         onBack: { dismiss() }) {
                LinkButton(title: zh ? "⌂ 首页" : "⌂ Home") { dismiss() }
            }
            ScrollView {
                VStack(alignment: .leading, spacing: 0) {
                    Text((zh ? "生效日期" : "Effective date") + ": 2026-09-08 · v1.0")
                        .font(.hkt(13))
                        .foregroundStyle(Theme.text2)
                        .frame(maxWidth: .infinity, alignment: .center)
                        .padding(.vertical, 2)
                        .padding(.bottom, 10)
                    ForEach(PrivacyContent.sections(zh: zh).indices, id: \.self) { index in
                        let section = PrivacyContent.sections(zh: zh)[index]
                        HKTCard {
                            VStack(alignment: .leading, spacing: 6) {
                                Text(section.title)
                                    .font(.hkt(14, .bold))
                                    .foregroundStyle(Theme.text)
                                Text(section.body)
                                    .font(.hkt(13))
                                    .lineSpacing(1.6 * 13 - 13)
                                    .foregroundStyle(Theme.text2)
                                    .frame(maxWidth: .infinity, alignment: .leading)
                            }
                        }
                        .padding(.bottom, 11)
                    }
                }
                .padding(.horizontal, 16)
                .padding(.bottom, 24)
            }
        }
        .background(Theme.bg)
        .toolbar(.hidden, for: .navigationBar)
    }
}

/// 原型 PRIVACY 对象的逐字移植（ZH L465-479 / EN L480-491）。
enum PrivacyContent {
    struct Section {
        let title: String
        let body: String
    }

    static func sections(zh: Bool) -> [Section] {
        zh ? zhSections : enSections
    }

    private static let zhSections: [Section] = [
        .init(title: "概述",
              body: "HKT BLETools（下称“本应用”）是在您的 iPhone 本地运行的设备管理工具，用于查找、连接和管理 HKT 蓝牙设备（MPS / SVC / UDS / EPS 系列）。\n本应用不收集、不上传、不共享任何个人数据，也不包含分析统计、广告或追踪组件。"),
        .init(title: "我们处理的信息（仅存于您的设备）",
              body: "• 蓝牙扫描数据：附近 HKT 设备的广播名称、系统标识符与信号强度，仅用于建立和维持连接；\n• 设备数据：所连接设备的固件版本、电量/电压、温湿度、经纬度（由设备上报）、阀门状态、地磁/雷达读数等状态与配置信息；\n• 本地记录：诊断日志、固件升级报告、最近连接的设备、语言与扫描过滤等设置。"),
        .init(title: "存储与保留",
              body: "上述数据仅保存在您的 iPhone 本地（系统沙盒）中，不会传输到任何服务器；删除本应用即同时永久删除上述全部数据。"),
        .init(title: "无网络传输与第三方",
              body: "本应用不具备任何服务器通信能力，不嵌入第三方分析、广告或追踪 SDK，不存在向外部第三方披露数据的行为。"),
        .init(title: "相机使用",
              body: "仅在您使用“定位设备 → 扫描设备二维码”功能时访问相机，用于读取设备标签上的二维码；相机画面不会被拍摄、录制或保存。"),
        .init(title: "蓝牙权限",
              body: "蓝牙权限仅用于扫描和连接 HKT 设备，不用于推断位置或建立行为画像。"),
        .init(title: "数据导出",
              body: "仅当您主动导出诊断日志或固件升级报告时，数据经 iOS 系统分享面板发送，目的地由您选择；在您操作之前，任何数据都不会离开设备。"),
        .init(title: "您的权利",
              body: "数据仅存于本机：删除全部数据 = 卸载本应用；导出数据 = 使用应用内导出功能。本应用无账号体系，不涉及在线行使权利的流程。"),
        .init(title: "儿童隐私",
              body: "本应用为工业/工程作业工具，不面向儿童，不有意收集任何个人信息。"),
        .init(title: "本说明的变更",
              body: "若未来版本的功能影响本说明（例如增加联网能力），我们将更新本页并标注版本与生效日期；重大变更会在应用内提示您。"),
        .init(title: "联系我们",
              body: "如对本说明有任何疑问，可通过邮件 sales@hktlora.com 或公司门户网站 www.hktlora.com 与我们联系。"),
    ]

    private static let enSections: [Section] = [
        .init(title: "Overview",
              body: "HKT BLETools (the “app”) is a device-management tool that runs locally on your iPhone to find, connect to and manage HKT Bluetooth devices (MPS / SVC / UDS / EPS series).\nThe app does not collect, upload or share any personal data, and contains no analytics, advertising or tracking components."),
        .init(title: "Information we process (stored only on your device)",
              body: "• Bluetooth scan data: advertised names, system identifiers and signal strength of nearby HKT devices, used only to establish and maintain connections;\n• Device data: firmware version, battery/voltage, temperature/humidity, latitude/longitude (reported by the device), valve states, magnetometer/radar readings and configuration of the connected device;\n• Local records: diagnostic logs, firmware-update reports, recently connected devices, language and scan-filter settings."),
        .init(title: "Storage and retention",
              body: "All data above is stored only on your iPhone (system sandbox) and is never transmitted to any server. Uninstalling the app permanently deletes all of it."),
        .init(title: "No network transfer, no third parties",
              body: "The app has no server-communication capability, embeds no third-party analytics, advertising or tracking SDKs, and does not disclose data to external parties."),
        .init(title: "Camera use",
              body: "The camera is accessed only when you use “Locate Device → Scan Device QR Code”, to read the QR code on the device label. The camera feed is never photographed, recorded or saved."),
        .init(title: "Bluetooth permission",
              body: "Bluetooth is used only to scan for and connect to HKT devices — never to infer your location or build behavioural profiles."),
        .init(title: "Data export",
              body: "Only when you actively export a diagnostic log or update report does data leave the device, via the iOS share sheet, to a destination you choose. Nothing leaves the device without your action."),
        .init(title: "Your rights",
              body: "Data lives only on this device: delete all data = uninstall the app; export data = use the in-app export. The app has no account system."),
        .init(title: "Children",
              body: "The app is an industrial/engineering tool, is not directed at children, and does not knowingly collect any personal information."),
        .init(title: "Changes to this statement",
              body: "If a future version changes what this statement covers (e.g. adding networking), we will update this page with a new version and date; material changes will be announced in the app."),
        .init(title: "Contact us",
              body: "If you have any questions about this statement, contact us at sales@hktlora.com or visit www.hktlora.com."),
    ]
}
