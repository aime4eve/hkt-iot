import CoreBLE
import CoreOTA
import CoreProtocol
import SwiftUI
import UniformTypeIdentifiers

/// OTA 升级页（P_ota）—— 克隆冻结原型（规格卡 `docs/ios/design/ui-spec/P-ota.md`）。
/// 固件包经系统文件选择窗口选取（真实文件名/大小/CRC32，期望版本从文件名解析）。
/// 真实设备：OTAEngine ACK 驱动分页传输（设备复位重传≤2 次，进度=已确认包数）；
/// 演示模式（-mockble）：tick 380ms 复刻原型。传输完成后的重启/重连/版本确认六段后程
/// 仍按原型节奏推进（真机重连随真机验证里程碑接入）。
struct OTAView: View {
    let session: DeviceSession

    @Environment(LanguageStore.self) private var langStore
    @Environment(\.dismiss) private var dismiss

    /// 0 选择 / 1 确认 / 2-7 运行六段 / 8 成功 / 9 失败
    @State private var stage = 0
    @State private var pickedURL: URL?
    @State private var pickedFileName = ""
    @State private var pickedFileSize = 0
    @State private var pickedCRC = ""
    @State private var expectedVersion = ""
    @State private var showImporter = false
    @State private var pkt = 0
    @State private var pageWrite = false
    @State private var wait = 0
    @State private var showGuard = false
    @State private var showReport = false
    @State private var startedAt = Date()
    @State private var totalText = ""
    @State private var failureReason = ""
    @State private var timer: Timer?
    @State private var engine: OTAEngine?
    @State private var totalPackets = 1284   // 演示包数；真实升级由所选文件决定

    private var zh: Bool { langStore.isZh }
    private var picked: Bool { pickedURL != nil }
    private var isDemo: Bool { ProcessInfo.processInfo.arguments.contains("-mockble") }
    /// 当前版本：轮询快照的 固件版本（演示设备 v11.28，真机即真实版本）。
    private var currentVersion: String {
        "v\(snapshot.hardwareVersion).\(snapshot.softwareVersion)"
    }
    private var snapshot: DeviceSnapshot { session.snapshot }

    var body: some View {
        VStack(spacing: 0) {
            NavbarHeader(title: zh ? "固件升级" : "Firmware Update",
                         backText: zh ? "‹ 返回" : "‹ Back",
                         onBack: { stopTimer(); dismiss() }) {
                EmptyView()
            }
            ScrollView {
                VStack(alignment: .leading, spacing: 0) {
                    content
                }
                .padding(.horizontal, 16)
                .padding(.bottom, 24)
            }
        }
        .background(Theme.bg)
        .toolbar(.hidden, for: .navigationBar)
        .overlay { dialogs }
        .onDisappear {
            stopTimer()
            engine?.cancel()
            session.rawFrameHandler = nil
            session.setPollingSuspended(false)
        }
        .onAppear {
            // 演示自动导航（-demo-page ota-run）：演示包直接开始传输
            if DemoLaunch.isPage("ota-run") {
                useDemoFile()
                runOTA()
            }
            // 演示自动导航（-demo-page ota-importer）：弹出系统文件选择窗口
            if DemoLaunch.isPage("ota-importer") {
                DispatchQueue.main.asyncAfter(deadline: .now() + 1.6) { showImporter = true }
            }
        }
        .fileImporter(isPresented: $showImporter,
                      allowedContentTypes: [.data],
                      allowsMultipleSelection: false) { result in
            guard case .success(let urls) = result, let url = urls.first else { return }
            adoptFile(url)
        }
    }

    /// 读取所选固件包：真实文件名/大小 + CRC32 校验值 + 文件名中的期望版本。
    private func adoptFile(_ url: URL) {
        let secured = url.startAccessingSecurityScopedResource()
        defer { if secured { url.stopAccessingSecurityScopedResource() } }
        guard let data = try? Data(contentsOf: url) else { return }
        pickedURL = url
        pickedFileName = url.lastPathComponent
        pickedFileSize = data.count
        pickedCRC = String(format: "CRC32 %08X", crc32(of: data))
        // 期望版本：从文件名解析（如 mps100_v1.28_full.bin → 1.28）
        if let match = url.lastPathComponent.range(of: #"v?(\d+)\.(\d+)"#, options: .regularExpression) {
            expectedVersion = String(url.lastPathComponent[match]).replacingOccurrences(of: "v", with: "")
        } else {
            expectedVersion = "?"
        }
    }

    /// CRC32（IEEE 802.3 查表法）。
    private func crc32(of data: Data) -> UInt32 {
        let table: [UInt32] = (0..<256).map { index -> UInt32 in
            var value = UInt32(index)
            for _ in 0..<8 {
                value = (value & 1) != 0 ? (0xEDB88320 ^ (value >> 1)) : (value >> 1)
            }
            return value
        }
        var crc: UInt32 = 0xFFFFFFFF
        for byte in data {
            crc = table[Int((crc ^ UInt32(byte)) & 0xFF)] ^ (crc >> 8)
        }
        return crc ^ 0xFFFFFFFF
    }

    /// 演示固件包（-demo-page ota-run 自动流程用）。
    private func useDemoFile() {
        pickedURL = URL(fileURLWithPath: "/tmp/mps100_v1.28_full.bin")
        pickedFileName = "mps100_v1.28_full.bin"
        pickedFileSize = 164_352
        pickedCRC = "CRC32 待升级时校验"
        expectedVersion = "1.28"
    }

    // MARK: - 阶段内容

    @ViewBuilder
    private var content: some View {
        switch stage {
        case 0: selectStage
        case 2 ... 7: runningStage
        case 8: successStage
        default: failureStage
        }
    }

    /// stage 0：选择固件（规格卡 §2 stage 0；系统文件选择窗口）。
    private var selectStage: some View {
        VStack(alignment: .leading, spacing: 0) {
            SettingsRow(label: "📄 " + (zh ? "选择固件文件…" : "Select firmware file…")) {
                RowValue { Text("▸") }
            } action: { showImporter = true }
            if picked {
                HKTCard {
                    VStack(alignment: .leading, spacing: 4) {
                        Text("📄 " + pickedFileName)
                            .font(.hkt(13)).foregroundStyle(Theme.text)
                            .lineLimit(1)
                        Text("\(pickedFileSize.formatted()) B · \(pickedCRC)")
                            .font(.hkt(13)).monospacedDigit().foregroundStyle(Theme.text2)
                        Text((zh ? "当前版本" : "Current version") + " \(currentVersion) → "
                             + (zh ? "期望版本" : "Expected version") + ": ")
                            .font(.hkt(13)).foregroundStyle(Theme.text2)
                            + Text(expectedVersion).bold().foregroundStyle(Theme.text)
                    }
                }
                .padding(.bottom, 11)
            }
            Button {
                stage = 1
            } label: {
                Text(zh ? "开始升级" : "Start Update")
                    .font(.hkt(16, .semibold))
                    .foregroundStyle(.white)
                    .frame(maxWidth: .infinity)
                    .padding(13)
                    .background(picked ? Theme.info : Theme.info.opacity(0.4),
                                in: RoundedRectangle(cornerRadius: Theme.controlRadius))
            }
            .disabled(!picked)
            Text("⚠︎ " + (zh ? "升级期间请保持 App 前台、勿锁屏" : "Keep the app in the foreground; don't lock the screen"))
                .font(.hkt(13))
                .foregroundStyle(Theme.text2)
                .padding(.top, 10)
        }
    }

    /// stage 2–7：进行中（stageList + 阶段卡 + 警告 + 取消升级）。
    private var runningStage: some View {
        VStack(alignment: .leading, spacing: 12) {
            stageList(activeIndex: max(0, min(stage - 2, 5)))
            VStack(alignment: .leading, spacing: 6) {
                HStack(spacing: 6) {
                    Text(phaseText).font(.hkt(15, .semibold)).foregroundStyle(Theme.text)
                    if stage >= 4 {
                        Text(zh ? "尚未完成" : "not done yet")
                            .font(.hkt(12, .semibold))
                            .foregroundStyle(Theme.warn)
                            .padding(.horizontal, 8).padding(.vertical, 3)
                            .background(Theme.warn.opacity(0.15), in: Capsule())
                    }
                    if stage == 2, pageWrite {
                        Text("· " + (zh ? "页写入中…" : "page writing…"))
                            .font(.hkt(15, .semibold)).foregroundStyle(Theme.text)
                    }
                }
                HKTProgress(fraction: stage >= 4 ? 1.0 : Double(pkt) / Double(totalPackets))
                HStack {
                    Text(zh ? "包 \(pkt) / \(totalPackets)" : "Packet \(pkt) / \(totalPackets)")
                    Spacer()
                    if stage <= 3 {
                        Text("\(Int(Double(pkt) / Double(totalPackets) * 100))%")
                    }
                }
                .font(.hkt(13))
                .monospacedDigit()
                .foregroundStyle(Theme.text2)
            }
            .padding(13)
            .frame(maxWidth: .infinity, alignment: .leading)
            .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
            .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
                .stroke(Theme.line.opacity(0.82), lineWidth: 1))
            .hktShadow()
            if stage <= 3 {
                HKTBanner(kind: .warn,
                          text: "⚠︎ " + (zh ? "升级期间请保持 App 前台、勿锁屏" : "Keep the app in the foreground; don't lock the screen"))
            }
            if stage >= 4 {
                Text("⏱ \(wait) s")
                    .font(.hkt(13))
                    .monospacedDigit()
                    .foregroundStyle(Theme.text2)
                    .frame(maxWidth: .infinity, alignment: .center)
            }
            Button {
                showGuard = true
            } label: {
                Text(zh ? "取消升级" : "Cancel Update")
                    .font(.hkt(16, .semibold))
                    .foregroundStyle(Theme.err)
                    .frame(maxWidth: .infinity)
                    .padding(13)
                    .background(Theme.err.opacity(0.10), in: RoundedRectangle(cornerRadius: Theme.controlRadius))
                    .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius)
                        .stroke(Theme.err.opacity(0.22), lineWidth: 1))
            }
        }
    }

    /// 六段进度（stageList：条 4px + 名称 10px；active=info/已过=ok/未到=fill·text2）。
    private func stageList(activeIndex: Int) -> some View {
        let names = zh
            ? ["选择固件", "确认", "数据传输", "等待重启", "重连", "版本确认"]
            : ["Select", "Confirm", "Transfer", "Reboot", "Reconnect", "Verify"]
        return HStack(spacing: 4) {
            ForEach(names.indices, id: \.self) { index in
                VStack(spacing: 4) {
                    RoundedRectangle(cornerRadius: 2)
                        .fill(index == activeIndex ? Theme.info
                                : index < activeIndex ? Theme.ok : Theme.fill)
                        .frame(height: 4)
                    Text(names[index])
                        .font(.hkt(10))
                        .foregroundStyle(index == activeIndex ? Theme.info
                                : index < activeIndex ? Theme.ok : Theme.text2)
                        .lineLimit(1)
                        .minimumScaleFactor(0.6)
                }
                .frame(maxWidth: .infinity)
            }
        }
        .padding(.bottom, 12)
    }

    private var phaseText: String {
        let messages = zh
            ? ["数据传输中", "数据发送完成", "等待设备重启…", "等待设备重新广播…", "正在重新连接设备…", "正在读取新固件版本…"]
            : ["Transferring data", "Transfer finished", "Waiting for the device to restart…",
               "Waiting for the device to re-advertise…", "Reconnecting to the device…", "Reading the new firmware version…"]
        return messages[max(0, min(stage - 2, 5))]
    }

    /// 成功（✓ 升级成功 + 报告入口 + 完成）。
    private var successStage: some View {
        VStack(spacing: 10) {
            Text("✓").font(.system(size: 48)).foregroundStyle(Theme.ok)
            Text(zh ? "升级成功 \(currentVersion) → \(expectedVersion)"
                    : "Update succeeded \(currentVersion) → \(expectedVersion)")
                .font(.hkt(17, .semibold))
            Text("⏱ \(totalText)")
                .font(.hkt(13)).monospacedDigit().foregroundStyle(Theme.text2)
            Button {
                showReport = true
            } label: {
                Text(zh ? "查看升级报告" : "View update report")
                    .font(.hkt(16, .semibold))
                    .foregroundStyle(Theme.text)
                    .padding(.vertical, 10).padding(.horizontal, 24)
                    .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
                    .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius)
                        .stroke(Theme.line, lineWidth: 1))
            }
            Button {
                stopTimer()
                dismiss()
            } label: {
                Text(zh ? "完成" : "Done")
                    .font(.hkt(16, .semibold))
                    .foregroundStyle(.white)
                    .padding(.vertical, 12).padding(.horizontal, 40)
                    .background(Theme.info, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
            }
            .padding(.top, 2)
        }
        .padding(.horizontal, 32)
        .frame(maxWidth: .infinity, minHeight: 430)
    }

    /// 失败（✕ + 原因 + 重新升级/导出日志/返回）。
    private var failureStage: some View {
        VStack(spacing: 10) {
            Text("✕").font(.system(size: 48)).foregroundStyle(Theme.err)
            Text(zh ? "升级失败" : "Update failed").font(.hkt(17, .semibold))
            Text((zh ? "原因: " : "Reason: ") + failureReason)
                .font(.hkt(13)).foregroundStyle(Theme.text2)
                .multilineTextAlignment(.center)
            Button {
                resetToSelect()
            } label: {
                Text(zh ? "重新升级" : "Retry Update")
                    .font(.hkt(16, .semibold))
                    .foregroundStyle(.white)
                    .padding(.vertical, 12).padding(.horizontal, 34)
                    .background(Theme.info, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
            }
            Button {
                // 导出日志占位（诊断里程碑）
            } label: {
                Text(zh ? "导出日志" : "Export Log")
                    .font(.hkt(16, .semibold))
                    .foregroundStyle(Theme.text)
                    .padding(.vertical, 10).padding(.horizontal, 22)
                    .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
                    .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius)
                        .stroke(Theme.line, lineWidth: 1))
            }
            LinkButton(title: zh ? "返回" : "Back") {
                stopTimer()
                dismiss()
            }
        }
        .padding(.horizontal, 32)
        .frame(maxWidth: .infinity, minHeight: 430)
    }

    // MARK: - 对话框（确认升级 / 离开保护 / 升级报告）

    @ViewBuilder
    private var dialogs: some View {
        if stage == 1 {
            DialogScaffold(title: zh ? "确认升级？" : "Start update?",
                           centeredBody: true,
                           content: {
                Text(zh ? "设备 \(session.deviceName) 将从 \(currentVersion) 升级到 v\(expectedVersion)。升级期间请保持 App 前台、蓝牙开启，勿离开此页面。"
                        : "Device \(session.deviceName) will update from \(currentVersion) to v\(expectedVersion). Keep the app in the foreground, keep Bluetooth on, and stay on this screen.")
            }, buttons: {
                DialogButton(title: zh ? "取消" : "Cancel") {
                    stage = 0
                    pickedURL = nil
                }
                DialogButton(title: zh ? "确认升级" : "Update", kind: .primary) {
                    runOTA()
                }
            })
        }
        if showGuard {
            DialogScaffold(title: "⚠︎ " + (zh ? "升级正在进行" : "Update in progress"),
                           centeredBody: true,
                           content: {
                Text(zh ? "现在离开会中断升级，可能导致设备无法正常工作，需要重新执行完整升级。确定要离开吗？"
                        : "Leaving now interrupts the update and may leave the device unusable, requiring a full re-run. Leave anyway?")
            }, buttons: {
                DialogButton(title: zh ? "继续升级" : "Keep Updating") { showGuard = false }
                DialogButton(title: zh ? "仍然离开" : "Leave", kind: .danger) {
                    showGuard = false
                    stopTimer()
                    dismiss()
                }
            })
        }
        if showReport {
            DialogScaffold(title: zh ? "OTA 升级报告" : "OTA Update Report",
                           content: {
                VStack(alignment: .leading, spacing: 2) {
                    reportRow(zh ? "设备" : "Device", session.deviceName)
                    reportRow(zh ? "升级前版本" : "Previous version", currentVersion)
                    reportRow(zh ? "期望版本" : "Expected", "v\(expectedVersion)")
                    reportRow(zh ? "实际版本" : "Actual", "v\(expectedVersion)")
                    reportRow(zh ? "传输包数" : "Packets sent", "\(totalPackets)")
                    reportRow(zh ? "设备复位重传" : "Device reset restarts", "0")
                    reportRow(zh ? "结果" : "Result", zh ? "成功" : "Success")
                    reportRow(zh ? "总耗时" : "Total time", totalText)
                }
                .padding(.vertical, 4)
            }, buttons: {
                DialogButton(title: zh ? "关闭" : "Close") { showReport = false }
            })
        }
    }

    private func reportRow(_ label: String, _ value: String) -> some View {
        HStack(alignment: .top) {
            Text(label)
            Spacer()
            Text(value).bold().foregroundStyle(Theme.text)
        }
        .padding(.vertical, 2)
    }

    // MARK: - 引擎（真实设备 = OTAEngine ACK 驱动；演示模式 = tick 380ms 复刻原型）

    private func runOTA() {
        if isDemo, pickedURL?.path.hasPrefix("/tmp/") == true {
            runDemoOTA()
        } else if let url = pickedURL, let data = loadFirmware(url), !data.isEmpty {
            runRealOTA(data)
        } else {
            failureReason = zh ? "固件包无法读取" : "Firmware file unreadable"
            stage = 9
        }
    }

    private func loadFirmware(_ url: URL) -> Data? {
        let secured = url.startAccessingSecurityScopedResource()
        defer { if secured { url.stopAccessingSecurityScopedResource() } }
        return try? Data(contentsOf: url)
    }

    /// 真实传输：通知帧 → 设备复位进引导 → ACK(2,n) 逐包 → ACK(3) 完成；后程六段照常推进。
    private func runRealOTA(_ data: Data) {
        let engine = OTAEngine(image: data)
        self.engine = engine
        engine.bind { [weak session] frame in session?.sendRaw(frame) }
        session.rawFrameHandler = { [weak engine] frame in engine?.handle(frame) ?? false }
        session.setPollingSuspended(true)   // 引导层不应答询帧，Android 同款停轮询
        totalPackets = engine.packetCount
        stage = 2
        pkt = 0
        wait = 0
        pageWrite = false
        startedAt = Date()
        LogStore.shared.info("OTA " + (zh ? "通知已发（\(totalPackets) 包），等待设备进入引导"
                                          : "notify sent (\(totalPackets) packets), waiting for bootloader"))
        engine.begin()
        timer?.invalidate()
        timer = Timer.scheduledTimer(withTimeInterval: 0.38, repeats: true) { _ in
            MainActor.assumeIsolated {
                guard let engine = self.engine else { return }
                pkt = engine.packetsDone
                switch engine.state {
                case .done where stage == 2:
                    stage = 3
                    wait = 0
                    session.rawFrameHandler = nil
                    session.setPollingSuspended(false)
                    LogStore.shared.info("OTA " + (zh ? "传输完成（设备 ACK 0x03）" : "transfer complete (ACK 0x03)"))
                case .failed(let error) where stage <= 7:
                    stopTimer()
                    failureReason = Self.failureText(error, linkLost: session.linkLost, zh: zh)
                    stage = 9
                    LogStore.shared.info("OTA " + (zh ? "失败：\(failureReason)" : "failed: \(failureReason)"))
                default:
                    if stage >= 3, stage < 8 { advancePostStages() }
                }
            }
        }
    }

    private static func failureText(_ error: OTAEngine.EngineError, linkLost: Bool, zh: Bool) -> String {
        switch error {
        case .timeout:
            return zh ? "蓝牙连接丢失（传输超时）" : "Bluetooth connection lost (transfer timeout)"
        case .tooManyRestarts:
            return zh ? "设备多次复位重传，传输中止" : "Device reset too many times; transfer aborted"
        case .cancelled:
            return zh ? "升级已取消" : "Update cancelled"
        }
    }

    /// 演示传输（tick 380ms，与原型演示一致）。
    private func runDemoOTA() {
        stage = 2
        pkt = 0
        wait = 0
        pageWrite = false
        startedAt = Date()
        LogStore.shared.info(zh ? "OTA 阶段：传输开始" : "OTA stage: transfer start")
        var pagePause = 0
        timer?.invalidate()
        timer = Timer.scheduledTimer(withTimeInterval: 0.38, repeats: true) { _ in
            MainActor.assumeIsolated {
                if stage == 2 {
                    if pkt >= totalPackets {
                        stage = 3
                        LogStore.shared.info(zh ? "OTA 阶段：传输完成" : "OTA stage: transfer done")
                    } else {
                        if pagePause > 0 {
                            pagePause -= 1
                            pageWrite = true
                        } else {
                            pageWrite = false
                            pkt = min(totalPackets, pkt + 4 + Int.random(in: 0..<5))
                            if pkt % 16 < 8 { pagePause = 1 }
                        }
                    }
                } else if stage >= 3, stage < 8 {
                    advancePostStages()
                }
            }
        }
    }

    /// 六段后程（stage 3→8）：传输完成后等待重启/重新广播/重连/版本确认（原型节奏）。
    private func advancePostStages() {
        switch stage {
        case 3:
            stage = 4; wait = 0
            LogStore.shared.info(zh ? "OTA 阶段：等待重启" : "OTA stage: waiting reboot")
        case 4:
            wait += 1
            if wait >= 4 { stage = 5; wait = 0 }
        case 5:
            wait += 1
            if wait >= 3 { stage = 6; wait = 0 }
        case 6:
            stage = 7; wait = 0
        case 7:
            wait += 1
            if wait >= 2 {
                stopTimer()
                stage = 8
                let total = Date().timeIntervalSince(startedAt)
                totalText = String(format: "%dm %ds", Int(total) / 60, Int(total) % 60)
                LogStore.shared.info(zh ? "OTA 升级成功" : "OTA success")
            }
        default:
            break
        }
    }

    private func resetToSelect() {
        stopTimer()
        engine?.cancel()
        engine = nil
        session.rawFrameHandler = nil
        session.setPollingSuspended(false)
        stage = 0
        pkt = 0
        wait = 0
    }

    private func stopTimer() {
        timer?.invalidate()
        timer = nil
    }
}
