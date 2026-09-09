import CoreBLE
import SwiftUI
import UIKit

struct RootView: View {
    @Environment(ScanModel.self) private var model
    @Environment(LanguageStore.self) private var langStore

    private var zh: Bool { langStore.isZh }

    var body: some View {
        switch model.availability {
        // P-08 ask 态：系统权限弹窗覆盖在门之上（原型自制弹窗由系统弹窗替代，规格卡 §5-1）
        case .initializing: GateView(icon: "🛡", title: zh ? "允许“HKT BLETools”使用蓝牙？" : "Allow “HKT BLETools” to use Bluetooth?",
                                     subtitle: zh ? "用于查找和连接附近的 HKT 设备" : "Used to find and connect nearby HKT devices",
                                     buttonTitle: nil, zh: zh)
        case .denied: GateView(icon: "🔒", title: zh ? "蓝牙权限未开启" : "Bluetooth permission is off",
                               subtitle: zh ? "需要在系统设置中允许蓝牙权限，才能扫描和连接设备。"
                                            : "Enable Bluetooth permission in Settings to scan and connect devices.",
                               buttonTitle: zh ? "去系统设置" : "Open Settings", zh: zh)
        case .poweredOff: GateView(icon: "📶", title: zh ? "蓝牙已关闭" : "Bluetooth is off",
                                   subtitle: zh ? "开启蓝牙后即可扫描附近设备。" : "Turn Bluetooth on to scan nearby devices.",
                                   buttonTitle: zh ? "打开蓝牙" : "Turn On Bluetooth", zh: zh)
        case .unsupported: GateView(icon: "⚠︎", title: zh ? "设备不支持蓝牙" : "Bluetooth unsupported",
                                    subtitle: zh ? "此 iPhone 不支持低功耗蓝牙（BLE）。" : "This iPhone doesn't support Bluetooth Low Energy.",
                                    buttonTitle: nil, zh: zh)   // 规格卡 §5-2：无按钮
        case .ready: ScanListView()
        }
    }

    private func openSystemSettings() {
        guard let url = URL(string: UIApplication.openSettingsURLString) else { return }
        UIApplication.shared.open(url)
    }
}

/// P-08 权限/蓝牙引导门（规格卡 P-08）。
/// 布局=内容弹性居中（图标 52 + 标题 17/600 + 副文案 13 + autoBack 行）+
/// 主按钮贴底全宽（用户 2026-09-10 裁决：与 P-02 取消同规格，复用 DialogButton）。
private struct GateView: View {
    let icon: String
    let title: String
    let subtitle: String
    let buttonTitle: String?   // nil = 无按钮（ask 由系统弹窗承担、unsup 无操作）
    let zh: Bool

    var body: some View {
        VStack(spacing: 0) {
            VStack(spacing: 10) {
                Text(icon).font(.system(size: 52))
                Text(title).font(.hkt(17, .semibold)).multilineTextAlignment(.center)
                Text(subtitle)
                    .font(.hkt(13))
                    .lineSpacing(1.5 * 13 - 13)
                    .foregroundStyle(Theme.text2)
                    .multilineTextAlignment(.center)
                if buttonTitle != nil {
                    Text(zh ? "授权后自动返回" : "Returns automatically once granted")
                        .font(.hkt(13))
                        .foregroundStyle(Theme.text2)
                }
            }
            .padding(.horizontal, 32)
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            if let buttonTitle {
                DialogButton(title: buttonTitle, kind: .primary) {
                    guard let url = URL(string: UIApplication.openSettingsURLString) else { return }
                    UIApplication.shared.open(url)
                }
                .padding(24)
            }
        }
        .background(Theme.bg)
    }
}

/// R-32 驻留详情占位：完整详情页随会话里程碑接入；本页验证"点击已连接设备直接回详情"。
struct ResidentDetailView: View {
    @Environment(ScanModel.self) private var model

    var body: some View {
        VStack(spacing: 12) {
            if let resident = model.residentDevice {
                Text(resident.name).font(.title2).fontWeight(.bold)
                Text("已连接").foregroundStyle(Theme.ok)
                Text("详情页随 M4 会话里程碑接入").font(.footnote).foregroundStyle(Theme.text2)
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Theme.bg)
        .navigationTitle("详情")
        .navigationBarTitleDisplayMode(.inline)
    }
}
