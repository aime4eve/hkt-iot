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

/// P-01 扫描列表（骨架版：能力就绪后的最小克隆；驻留徽章/定位入口随后续里程碑接入）。
private struct ScanListView: View {
    @Environment(ScanModel.self) private var model

    var body: some View {
        NavigationStack {
            content
                .navigationTitle("HKT BLETools")
                .navigationBarTitleDisplayMode(.inline)
        }
        .background(Theme.bg)
    }

    @ViewBuilder
    private var content: some View {
        if model.isScanning || !model.devices.isEmpty {
            List {
                Section {
                    statusRow
                    ForEach(model.devices) { device in
                        deviceCard(device)
                    }
                }
            }
            .listStyle(.insetGrouped)
            .scrollContentBackground(.hidden)
        } else {
            VStack(spacing: 12) {
                Text("📡").font(.system(size: 44))
                Text("未发现支持设备").font(.headline)
                Text("请确认设备已上电、在信号范围内").font(.subheadline).foregroundStyle(Theme.text2)
                Button("重新扫描") { model.startScan() }
                    .buttonStyle(.bordered)
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
    }

    private var statusRow: some View {
        HStack {
            Text(model.isScanning
                 ? "扫描中… 已发现 \(model.devices.count) 台"
                 : "扫描完成 · \(model.devices.count) 台")
            Spacer()
            Button(model.isScanning ? "停止" : "重新扫描") {
                model.isScanning ? model.stopScan() : model.startScan()
            }
        }
        .font(.subheadline)
    }

    private func deviceCard(_ device: DiscoveredDevice) -> some View {
        HStack {
            VStack(alignment: .leading, spacing: 2) {
                Text(device.name).fontWeight(.semibold)
                Text("ID \(device.identifier.uuidString.suffix(6)) · \(device.rssi) dBm")
                    .font(.caption).foregroundStyle(Theme.text2)
            }
            Spacer()
        }
    }
}
