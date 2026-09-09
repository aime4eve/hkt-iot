import CoreBLE
import SwiftUI

/// P-07 设置页 —— 1:1 克隆冻结原型（规格卡 `docs/ios/design/ui-spec/P-07_settings_log.md`）。
/// 语言（三态循环）/ 诊断日志入口 / 扫描过滤（阈值滑杆 + 前缀芯片）/ 关于（版本 7 击调试 + 隐私入口）。
struct SettingsView: View {
    @Environment(ScanModel.self) private var scanModel
    @Environment(LanguageStore.self) private var langStore
    @Environment(\.dismiss) private var dismiss
    @State private var showLog = false
    @State private var debugTaps = 0

    private var zh: Bool { langStore.isZh }

    var body: some View {
        VStack(spacing: 0) {
            NavbarHeader(title: zh ? "设置" : "Settings",
                         backText: zh ? "‹ 返回" : "‹ Back",
                         onBack: { dismiss() }) {
                LinkButton(title: zh ? "⌂ 首页" : "⌂ Home") { dismiss() }
            }
            ScrollView {
                settingsContent
            }
        }
        .background(Theme.bg)
        .toolbar(.hidden, for: .navigationBar)
        .navigationDestination(isPresented: $showLog) {
            LogView()
        }
    }

    private var settingsContent: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 0) {
                // 语言
                SectionHeader(title: zh ? "语言" : "Language")
                SettingsRow(label: zh ? "语言" : "Language") {
                    RowValue {
                        Text(langStore.label)
                        Text("▸")
                    }
                } action: { langStore.cycle() }

                // 诊断日志
                SectionHeader(title: zh ? "诊断日志" : "Diagnostic Log")
                SettingsRow(label: zh ? "诊断日志" : "Diagnostic Log") {
                    RowValue {
                        Text(zh ? "查看 ▸" : "View ▸")
                    }
                } action: { showLog = true }

                // 扫描过滤
                SectionHeader(title: zh ? "扫描过滤" : "Scan filter")
                SettingsRow(label: zh ? "信号强度阈值" : "Signal threshold") {
                    RowValue {
                        Slider(value: Binding(
                            get: { Double(scanModel.rssiThreshold) },
                            set: { scanModel.rssiThreshold = Int($0) }
                        ), in: -95 ... -40, step: 5)
                        .tint(Theme.info)
                        .frame(width: 110)
                        Text("\(scanModel.rssiThreshold) dBm").fixedSize()
                    }
                }
                HKTCard {
                    VStack(alignment: .leading, spacing: 0) {
                        Text(zh ? "设备名称前缀过滤" : "Name prefix filter")
                            .font(.hkt(12))
                            .foregroundStyle(Theme.text2)
                        HStack(spacing: 6) {
                            ForEach(["MPS", "SVC", "UDS", "EPS"], id: \.self) { prefix in
                                PrefixChip(text: prefix,
                                           selected: scanModel.allowedPrefixes.contains(prefix)) {
                                    scanModel.togglePrefix(prefix)
                                }
                            }
                        }
                        .padding(.top, 8)
                        Text(zh ? "仅显示名称以所选前缀开头的设备；未命名设备自动排除"
                                : "Only devices whose name starts with a selected prefix; unnamed devices are excluded")
                            .font(.hkt(11))
                            .foregroundStyle(Theme.text2)
                            .padding(.top, 6)
                    }
                }
                .padding(.bottom, 9)

                // 关于
                SectionHeader(title: zh ? "关于" : "About")
                SettingsRow(label: zh ? "版本" : "Version") {
                    RowValue { Text(versionText) }
                } action: { registerDebugTap() }
                SettingsRow(label: zh ? "隐私说明" : "Privacy") {
                    RowValue { Text("▸") }
                } action: { /* 隐私页（P_privacy 全文）随下一批接入 */ }
                if debugTaps > 0, debugTaps < 7 {
                    Text(zh ? "再点 \(7 - debugTaps) 次开启调试模式" : "\(7 - debugTaps) more taps to enable Debug")
                        .font(.hkt(13))
                        .foregroundStyle(Theme.text2)
                        .frame(maxWidth: .infinity, alignment: .center)
                        .padding(.top, 6)
                }
                if debugTaps >= 7 {
                    Text(zh ? "已开启调试模式" : "Debug mode enabled")
                        .font(.hkt(13))
                        .foregroundStyle(Theme.ok)
                        .frame(maxWidth: .infinity, alignment: .center)
                        .padding(.top, 6)
                }
            }
            .padding(.horizontal, 16)
            .padding(.bottom, 24)
        }
    }

    private var versionText: String {
        let info = Bundle.main.infoDictionary
        let version = info?["CFBundleShortVersionString"] as? String ?? "1.0.0"
        let build = info?["CFBundleVersion"] as? String ?? "123"
        return "\(version) (\(build))"
    }

    /// 原型 taps()：连点版本行 7 次开启调试模式。
    private func registerDebugTap() {
        debugTaps += 1
    }
}

/// P-07b 诊断日志页 —— navbar.small（导出）+ 日志行列表 / 空态。
struct LogView: View {
    @Environment(\.dismiss) private var dismiss

    private var zh: Bool { LanguageStore.shared.isZh }

    var body: some View {
        VStack(spacing: 0) {
            NavbarHeader(title: zh ? "诊断日志" : "Diagnostic Log",
                         backText: zh ? "‹ 返回" : "‹ Back",
                         onBack: { dismiss() }) {
                HStack(spacing: 8) {
                    LinkButton(title: zh ? "⌂ 首页" : "⌂ Home") { dismiss() }
                    LinkButton(title: zh ? "导出" : "Export") { /* 真实 txt 导出随诊断里程碑接入 */ }
                }
            }
            logContent
        }
        .background(Theme.bg)
        .toolbar(.hidden, for: .navigationBar)
    }

    @ViewBuilder
    private var logContent: some View {
        let entries = LogStore.shared.entries
        if entries.isEmpty {
            VStack {
                Text(zh ? "暂无日志" : "No entries yet")
                    .font(.hkt(13))
                    .foregroundStyle(Theme.text2)
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(Theme.bg)
        } else {
            ScrollView {
                VStack(spacing: 0) {
                    ForEach(entries) { entry in
                        LogLine(timestamp: entry.timestamp, level: entry.level, message: entry.message)
                    }
                }
                .padding(.horizontal, 16)
                .padding(.bottom, 24)
            }
        }
    }
}
