import CoreBLE
import SwiftUI
import UIKit

struct RootView: View {
    @Environment(ScanModel.self) private var model

    var body: some View {
        switch model.availability {
        case .initializing: GateView(icon: "📡", title: "正在初始化蓝牙…", message: nil, actionTitle: nil, action: nil)
        case .denied: GateView(icon: "🔒", title: "蓝牙权限未开启",
                               message: "需要在系统设置中允许蓝牙权限，才能扫描和连接设备。",
                               actionTitle: "去系统设置", action: { openSystemSettings() })
        case .poweredOff: GateView(icon: "📶", title: "蓝牙已关闭",
                                   message: "开启蓝牙后即可扫描附近设备。",
                                   actionTitle: "打开蓝牙", action: { openSystemSettings() })
        case .unsupported: GateView(icon: "⚠️", title: "设备不支持蓝牙",
                                    message: "此 iPhone 不支持低功耗蓝牙（BLE）。",
                                    actionTitle: nil, action: nil)
        case .ready: ScanListView()
        }
    }

    private func openSystemSettings() {
        guard let url = URL(string: UIApplication.openSettingsURLString) else { return }
        UIApplication.shared.open(url)
    }
}

/// P-08 权限/蓝牙引导（四态）。
private struct GateView: View {
    let icon: String
    let title: String
    let message: String?
    let actionTitle: String?
    let action: (() -> Void)?

    var body: some View {
        VStack(spacing: 12) {
            Text(icon).font(.system(size: 52))
            Text(title).font(.headline)
            if let message { Text(message).font(.subheadline).foregroundStyle(Theme.text2) }
            if let actionTitle, let action {
                Button(actionTitle, action: action)
                    .buttonStyle(.borderedProminent)
                    .padding(.top, 6)
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
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

