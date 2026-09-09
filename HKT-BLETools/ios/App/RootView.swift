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

/// P-01 扫描列表：含 ⌖ 目标设备定位入口（R-2）。
private struct ScanListView: View {
    @Environment(ScanModel.self) private var model
    @State private var connector: ConnectModel?
    @State private var showResidentDetail = false
    @State private var showLocate = false

    var body: some View {
        NavigationStack {
            content
                .navigationTitle("HKT BLETools")
                .navigationBarTitleDisplayMode(.inline)
                .toolbar {
                    ToolbarItem(placement: .topBarTrailing) {
                        Button {
                            model.beginLocateInput()
                            showLocate = true
                        } label: {
                            Image(systemName: "location.magnifyingglass")
                        }
                        .accessibilityLabel("定位设备")
                    }
                }
                // fullScreenCover 不继承环境对象，必须显式重新注入（否则连接弹层访问 ScanModel 即崩）
                .fullScreenCover(item: $connector) { connector in
                    ConnectOverlayView(model: connector)
                        .environment(model)
                }
                .fullScreenCover(isPresented: $showLocate) {
                    LocateFlowView()
                        .environment(model)
                }
                .navigationDestination(isPresented: $showResidentDetail) {
                    if let session = model.activeSession {
                        DeviceDetailView(session: session) { model.disconnectActive() }
                    } else {
                        ResidentDetailView()
                    }
                }
                .onReceive(NotificationCenter.default.publisher(for: .init("popToRoot"))) { _ in
                    showResidentDetail = false
                }
                .onChange(of: model.locateHitDevice) { _, hit in
                    guard let hit else { return }
                    connector = model.connector(for: hit)
                }
                .onChange(of: model.requestShowDetail) { _, request in
                    if request {
                        model.requestShowDetail = false
                        showResidentDetail = true
                    }
                }
        }
        .background(Theme.bg)
    }

    @ViewBuilder
    private var content: some View {
        if model.isScanning || !model.devices.isEmpty || model.residentDevice != nil {
            List {
                Section {
                    statusRow
                    // R-32：驻留会话设备置顶带"已连接"徽章，点击直接回详情（不重连）
                    if let resident = model.residentDevice {
                        Button {
                            showResidentDetail = true
                        } label: {
                            HStack {
                                VStack(alignment: .leading, spacing: 2) {
                                    HStack(spacing: 6) {
                                        Text(resident.name).fontWeight(.semibold)
                                        Text("已连接").font(.caption2).fontWeight(.bold)
                                            .padding(.horizontal, 6).padding(.vertical, 2)
                                            .background(Theme.ok.opacity(0.15), in: Capsule())
                                            .foregroundStyle(Theme.ok)
                                    }
                                    Text("ID …\(resident.identifier.uuidString.suffix(6))")
                                        .font(.caption).foregroundStyle(Theme.text2)
                                }
                                Spacer()
                                Image(systemName: "chevron.right").font(.caption).foregroundStyle(Theme.text2)
                            }
                        }
                        .buttonStyle(.plain)
                    }
                    ForEach(model.devices) { device in
                        if device.identifier != model.residentDevice?.identifier {
                            Button {
                                model.stopScan()               // 安卓同款：连接前先停扫
                                connector = model.connector(for: device)
                            } label: {
                                deviceCard(device)
                            }
                            .buttonStyle(.plain)
                        }
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

/// P-02 连接过程覆盖层（SP-4 三阶段 + 取消 + 失败原因/重试）。
struct ConnectOverlayView: View {
    let model: ConnectModel
    @Environment(\.dismiss) private var dismiss
    @Environment(ScanModel.self) private var scanModel
    @State private var dismissed = false

    private let phaseTexts = ["正在连接", "正在发现服务…", "正在订阅通知…"]

    var body: some View {
        ZStack {
            Color.black.opacity(0.42).ignoresSafeArea()
            VStack(alignment: .leading, spacing: 16) {
                Text(model.target.name).font(.headline)
                steps
                if model.outcome == nil {
                    Text(phaseTexts[min(model.phaseIndex, 2)])
                        .font(.subheadline).foregroundStyle(Theme.text2)
                    Button {
                        model.cancel()
                        // 立即关覆盖层：取消=预期动作，不留在冻结的中间态卡片
                        dismissed = true
                        dismiss()
                    } label: {
                        Text("取消")
                            .frame(maxWidth: .infinity).frame(height: 44)
                            .background(Theme.card2, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
                            .foregroundStyle(Theme.err).font(.subheadline).fontWeight(.semibold)
                    }
                    .buttonStyle(.plain)
                } else if case .failed(let failure) = model.outcome {
                    Image(systemName: "exclamationmark.triangle.fill").foregroundStyle(Theme.err)
                    Text("连接失败：\(model.failureText ?? "未知原因")").font(.subheadline)
                    Button("重新连接") { model.retry() }
                        .buttonStyle(.borderedProminent)
                    Button("返回") { finish() }
                        .buttonStyle(.bordered)
                }
            }
            .padding(22)
            .background(Theme.card, in: RoundedRectangle(cornerRadius: 18))
            .padding(40)
        }
        .onAppear { model.start() }   // 关键：弹出即启动三阶段连接（此前从未调用，连接永远不发）
        .onChange(of: model.outcome) { _, outcome in
            guard let outcome, !dismissed else { return }
            switch outcome {
            case .connected:
                scanModel.residentDevice = ResidentDevice(
                    name: model.target.name,
                    identifier: model.target.identifier)
                scanModel.makeAndStartSession(for: model.target)   // R-5：连接成功即启动会话轮询
                scanModel.requestShowDetail = true                 // 覆盖层关闭后自动进入详情页
                dismissed = true
                dismiss()
            case .cancelled:
                // 用户主动取消：直接关闭覆盖层回扫描页（不做驻留）
                dismissed = true
                dismiss()
            case .failed:
                break   // 失败：留在原地展示原因与重试/返回
            }
        }
    }

    private var steps: some View {
        HStack(spacing: 6) {
            ForEach(0..<3, id: \.self) { index in
                Capsule()
                    .fill(index <= model.phaseIndex ? Theme.info : Theme.fill)
                    .frame(height: 4)
            }
        }
    }

    private func finish() {
        dismissed = true
        dismiss()
    }
}
