import CoreBLE
import SwiftUI

/// P-01 扫描页 —— 1:1 克隆冻结原型（规格卡 `docs/ios/design/ui-spec/P-01.md`）。
/// 结构：大标题页头（⌖ 定位 + ⚙︎）→ 滚动区（扫描状态行/驻留卡/设备卡/空态/最近设备）→ 切换确认框。
/// 交互=R-31/R-32：点驻留设备回详情、点其他设备弹切换确认（原子释放）、重扫带健康检测。
struct ScanListView: View {
    @Environment(ScanModel.self) private var model
    @Environment(LanguageStore.self) private var langStore
    @State private var connector: ConnectModel?
    @State private var showResidentDetail = false
    @State private var showLocate = false
    @State private var showSettings = false

    private var zh: Bool { langStore.isZh }

    var body: some View {
        NavigationStack {
            VStack(spacing: 0) {
                NavbarLarge(title: "HKT BLETools") {
                    LinkButton(title: zh ? "⌖ 定位" : "⌖ Locate") {
                        model.beginLocateInput()
                        showLocate = true
                    }
                    .padding(.trailing, 12)
                    Button("⚙︎") { showSettings = true }
                        .font(.hkt(14, .semibold))
                        .foregroundStyle(Theme.info)
                        .padding(.horizontal, 9).padding(.vertical, 6)
                        .background(Theme.info.opacity(0.09), in: RoundedRectangle(cornerRadius: Theme.controlRadius))
                }
                ScrollView {
                    VStack(alignment: .leading, spacing: 0) {
                        statusRow
                        content
                    }
                    .padding(.horizontal, 16)
                    .padding(.bottom, 24)
                }
            }
            .background(Theme.bg)
            .toolbar(.hidden, for: .navigationBar)      // 原型用自绘大标题页头
            // fullScreenCover 不继承环境对象，必须显式重新注入（否则弹层访问 ScanModel 即崩）
            .fullScreenCover(item: $connector) { connector in
                ConnectOverlayView(model: connector)
                    .environment(model)
                    .environment(langStore)
            }
            .fullScreenCover(isPresented: $showLocate) {
                LocateFlowView()
                    .environment(model)
                    .environment(langStore)
            }
            .navigationDestination(isPresented: $showSettings) {
                SettingsView()
            }
            .navigationDestination(isPresented: $showResidentDetail) {
                if let session = model.activeSession {
                    // R-31：确认断开=原子释放并回扫描页（列表保留，设备进最近设备）
                    DeviceDetailView(session: session) {
                        model.disconnectActive()
                        NotificationCenter.default.post(name: .init("popToRoot"), object: nil)
                    }
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
            .overlay { switchDialog }
        }
    }

    // MARK: - 扫描状态行（规格卡 §2.2）

    private var statusRow: some View {
        ScanStatusRow(title: model.isScanning
            ? String(format: zh ? "扫描中… 已发现 %d 台" : "Scanning… %d found", model.devices.count)
            : String(format: zh ? "扫描完成 · %d 台" : "Scan finished · %d found", model.devices.count),
            actionTitle: model.isScanning ? (zh ? "停止" : "Stop") : (zh ? "重新扫描" : "Rescan")) {
            model.isScanning ? model.stopScan() : model.startScan()
        }
    }

    // MARK: - 列表内容（规格卡 §1 DOM 顺序）

    @ViewBuilder
    private var content: some View {
        let residentActive = model.residentDevice != nil && model.activeSession != nil
        if !model.isScanning, model.devices.isEmpty, !residentActive {
            CenterStateView(glyph: "📡", glyphSize: 44,
                            title: zh ? "未发现支持设备" : "No supported devices found",
                            subtitle: zh ? "请确认设备已上电、在信号范围内" : "Make sure devices are powered and nearby",
                            buttonTitle: zh ? "重新扫描" : "Rescan",
                            secondaryButton: true, fillsRemaining: false) {
                model.startScan()
            }
            recentSection
        } else {
            if let resident = model.residentDevice, residentActive {
                ScanDeviceCard(name: resident.name,
                               subtitle: "ID …\(idSuffix(resident.identifier))",
                               badge: StateBadge(kind: .ok, text: zh ? "已连接" : "Connected", compact: true)) {
                    Text("›").font(.hkt(14, .semibold)).foregroundStyle(Theme.info)
                }
                .onTapGesture { tapCard(resident.identifier, resident.name) }
                .padding(.bottom, 11)
            }
            ForEach(model.devices) { device in
                if device.identifier != model.residentDevice?.identifier {
                    ScanDeviceCard(name: device.name,
                                   subtitle: "ID …\(idSuffix(device.identifier)) · \(device.rssi) dBm") {
                        RssiBars(lit: rssiLit(device.rssi))
                    }
                    .onTapGesture { tapCard(device.identifier, device.name) }
                    .padding(.bottom, 11)
                }
            }
            if !residentActive, let last = model.lastSession, model.devices.isEmpty {
                recentCard(last)
            }
        }
    }

    /// 最近设备（规格卡 §2.5：section + 卡片，仅空列表 && 无驻留时）。
    @ViewBuilder
    private var recentSection: some View {
        if let last = model.lastSession {
            SectionHeader(title: zh ? "最近设备" : "Recent")
            recentCard(last)
        }
    }

    private func recentCard(_ last: ResidentDevice) -> some View {
        ScanDeviceCard(name: last.name,
                       subtitle: zh ? "ID …\(idSuffix(last.identifier)) · 昨天" : "ID …\(idSuffix(last.identifier)) · Yesterday") {
            Text("↻").font(.hkt(14, .semibold)).foregroundStyle(Theme.info)
        }
        .onTapGesture { connector = model.connectorForLastSession() }
        .padding(.bottom, 11)
    }

    // MARK: - 点击语义（deviceCardClick 三分支，规格卡 §1）

    private func tapCard(_ identifier: UUID, _ name: String) {
        guard let device = model.devices.first(where: { $0.identifier == identifier })
            ?? residentAsDiscovered(identifier, name) else { return }
        switch model.cardTapOutcome(for: device) {
        case .showDetail:
            showResidentDetail = true
        case .confirmSwitch(let target):
            model.requestSwitch(to: target)
        case .connect(let connect):
            connector = connect
        }
    }

    /// 驻留设备可能不在扫描列表（列表冻结/停扫），点击时按驻留信息还原。
    private func residentAsDiscovered(_ identifier: UUID, _ name: String) -> DiscoveredDevice? {
        guard model.residentDevice?.identifier == identifier else { return nil }
        return DiscoveredDevice(name: name, identifier: identifier, rssi: Int.min)
    }

    // MARK: - 切换确认框（R-32，规格卡 §2.6）

    @ViewBuilder
    private var switchDialog: some View {
        if let target = model.pendingSwitch, let current = model.residentDevice {
            let message = Text(zh ? "当前已连接 " : "Currently connected to ")
                + Text(current.name).bold()
                + Text(" …\(idSuffix(current.identifier))" + (zh ? "。" : ". "))
                + Text(zh ? "切换将断开当前会话并连接 " : "Switching will disconnect it and connect to ")
                + Text(target.name).bold()
                + Text(" …\(idSuffix(target.identifier))" + (zh ? "。" : "."))
            HKTDialog(title: zh ? "切换设备？" : "Switch device?",
                      message: message,
                      buttons: {
                DialogButton(title: zh ? "取消" : "Cancel") { model.cancelSwitch() }
                DialogButton(title: zh ? "切换并连接" : "Switch & Connect", kind: .primary) {
                    connector = model.confirmSwitch()
                }
            })
        }
    }

    // MARK: - 通用

    private func idSuffix(_ identifier: UUID) -> String {
        String(identifier.uuidString.suffix(4)).uppercased()
    }

    /// 原型映射：rssi > -70 → 4 亮；> -85 → 3 亮；否则 2 亮。
    private func rssiLit(_ rssi: Int) -> Int {
        rssi > -70 ? 4 : (rssi > -85 ? 3 : 2)
    }
}
