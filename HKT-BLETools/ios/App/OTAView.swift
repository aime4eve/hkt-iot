import CoreBLE
import CoreProtocol
import SwiftUI

/// OTA 升级页（P_ota）—— 1:1 克隆冻结原型（规格卡 `docs/ios/design/ui-spec/P-ota.md`）。
/// 六阶段演示引擎（tick 380ms 复刻原型）；真实 OTAEngine 随协议里程碑接入。
struct OTAView: View {
    let session: DeviceSession

    @Environment(LanguageStore.self) private var langStore
    @Environment(\.dismiss) private var dismiss

    /// 0 选择 / 1 确认 / 2-7 运行六段 / 8 成功 / 9 失败
    @State private var stage = 0
    @State private var picked = false
    @State private var pkt = 0
    @State private var pageWrite = false
    @State private var wait = 0
    @State private var showGuard = false
    @State private var showReport = false
    @State private var startedAt = Date()
    @State private var totalText = ""
    @State private var timer: Timer?

    private let totalPackets = 1284
    private let fileURL = URL(fileURLWithPath: "/tmp/mps100_v1.28_full.bin")

    private var zh: Bool { langStore.isZh }

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
        .onDisappear { stopTimer() }
        .onAppear {
            // 演示自动导航（-demo-page ota-run）：选中文件并直接开始传输
            if DemoLaunch.isPage("ota-run") {
                picked = true
                runOTA()
            }
        }
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

    /// stage 0：选择固件（规格卡 §2 stage 0）。
    private var selectStage: some View {
        VStack(alignment: .leading, spacing: 0) {
            SettingsRow(label: "📄 " + (zh ? "选择固件文件…" : "Select firmware file…")) {
                RowValue { Text("▸") }
            } action: { picked = true }
            if picked {
                HKTCard {
                    VStack(alignment: .leading, spacing: 4) {
                        Text("📄 " + (zh ? "mps100_v1.28_full.bin" : "mps100_v1.28_full.bin"))
                            .font(.hkt(13)).foregroundStyle(Theme.text)
                        Text("164,352 B · CRC ✓")
                            .font(.hkt(13)).monospacedDigit().foregroundStyle(Theme.text2)
                        Text((zh ? "当前版本" : "Current version") + " v1.26 → "
                             + (zh ? "期望版本" : "Expected version") + ": ")
                            .font(.hkt(13)).foregroundStyle(Theme.text2)
                            + Text("1.28").bold().foregroundStyle(Theme.text)
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
            Text(zh ? "升级成功 1.26 → 1.28" : "Update succeeded 1.26 → 1.28")
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
            Text((zh ? "原因: " : "Reason: ") + (zh ? "蓝牙连接丢失" : "Bluetooth connection lost"))
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
                Text(zh ? "设备 \(session.deviceName) 将从 v1.26 升级到 v1.28。升级期间请保持 App 前台、蓝牙开启，勿离开此页面。"
                        : "Device \(session.deviceName) will update from v1.26 to v1.28. Keep the app in the foreground, keep Bluetooth on, and stay on this screen.")
            }, buttons: {
                DialogButton(title: zh ? "取消" : "Cancel") {
                    stage = 0
                    picked = false
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
                    reportRow(zh ? "升级前版本" : "Previous version", "v1.26")
                    reportRow(zh ? "期望版本" : "Expected", "v1.28")
                    reportRow(zh ? "实际版本" : "Actual", "v1.28")
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

    // MARK: - 演示引擎（tick 380ms 复刻原型；真实 OTAEngine 随协议里程碑接入）

    private func runOTA() {
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
                } else if stage == 3 {
                    stage = 4; wait = 0
                    LogStore.shared.info(zh ? "OTA 阶段：等待重启" : "OTA stage: waiting reboot")
                } else if stage == 4 {
                    wait += 1
                    if wait >= 4 { stage = 5; wait = 0 }
                } else if stage == 5 {
                    wait += 1
                    if wait >= 3 { stage = 6; wait = 0 }
                } else if stage == 6 {
                    stage = 7; wait = 0
                } else if stage == 7 {
                    wait += 1
                    if wait >= 2 {
                        stopTimer()
                        stage = 8
                        let total = Date().timeIntervalSince(startedAt)
                        totalText = String(format: "%dm %ds", Int(total) / 60, Int(total) % 60)
                        LogStore.shared.info(zh ? "OTA 升级成功 v1.28" : "OTA success v1.28")
                    }
                }
            }
        }
    }

    private func resetToSelect() {
        stopTimer()
        stage = 0
        picked = true
        pkt = 0
        wait = 0
    }

    private func stopTimer() {
        timer?.invalidate()
        timer = nil
    }
}
