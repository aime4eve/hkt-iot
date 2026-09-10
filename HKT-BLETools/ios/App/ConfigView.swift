import CoreBLE
import CoreProtocol
import SwiftUI

/// 参数配置页（P_config）—— 1:1 克隆冻结原型（规格卡 `docs/ios/design/ui-spec/P-config.md`）。
/// 三家族三套表单 + 前置校验（与固件同规则）+ 确认摘要 + 写入中 + 结果横幅。
/// 保存走真实 0x02 写入：设备 ACK（含 0xFF 应答段的专用帧）驱动结果横幅。
struct ConfigView: View {
    let session: DeviceSession

    @Environment(LanguageStore.self) private var langStore
    @Environment(\.dismiss) private var dismiss

    @State private var draft = ConfigDraft()
    @State private var errors: [String: String] = [:]
    @State private var banner: BannerKind?
    @State private var showConfirm = false
    @State private var showSaving = false
    @State private var tzHint: String?
    @State private var initialized = false

    enum BannerKind: Equatable { case ok, fail }

    private var snapshot: DeviceSnapshot { session.snapshot }
    private var zh: Bool { langStore.isZh }
    private var isUDS: Bool { session.family == .uds100 }
    private var isSVC: Bool { session.family == .svc100 }

    var body: some View {
        VStack(spacing: 0) {
            NavbarHeader(title: zh ? "参数配置" : "Configuration",
                         backText: zh ? "‹ 返回" : "‹ Back",
                         onBack: { dismiss() }) {
                EmptyView()
            }
            if isSVC {
                // SVC 专属：表单滚动 + 吸底 savebar（规格卡 §1）
                ScrollView {
                    VStack(alignment: .leading, spacing: 0) {
                        if let banner { resultBanner(banner) }
                        svcForm
                    }
                    .padding(.horizontal, 16)
                    .padding(.top, 12)
                }
                saveBar
            } else {
                ScrollView {
                    VStack(alignment: .leading, spacing: 0) {
                        if let banner { resultBanner(banner) }
                        formBody
                        saveArea
                    }
                    .padding(.horizontal, 16)
                    .padding(.bottom, 24)
                }
            }
        }
        .background(Theme.bg)
        .toolbar(.hidden, for: .navigationBar)
        .overlay { confirmDialog }
        .overlay { savingDialog }
        .onAppear {
            guard !initialized else { return }
            initialized = true
            initDraft()
        }
    }

    /// SVC 吸底保存栏（.savebar：bg、上边框 line 68%、payload mono 居中）。
    private var saveBar: some View {
        VStack(spacing: 7) {
            saveButton
            Text(payloadText)
                .font(.system(size: 11, weight: .regular, design: .monospaced))
                .foregroundStyle(Theme.text2)
        }
        .padding(EdgeInsets(top: 11, leading: 16, bottom: 24, trailing: 16))
        .frame(maxWidth: .infinity)
        .background(Theme.bg)
        .overlay(alignment: .top) {
            Rectangle().fill(Theme.line.opacity(0.68)).frame(height: 1)
        }
    }

    // MARK: - 草稿（cfgInit：初值=轮询快照）

    struct ConfigDraft {
        var report = ""
        var gps = ""
        var low = ""
        var high = ""
        var modeIndex = 0        // dc 工作模式
        var vol = 2              // svc 电压档 0/1/2
        var port = 1             // svc 端口原始值 0x00-0x03 | 0x80
        var stable = "5"
        var smart = false
        var tz = 0
        var period = ""
    }

    private func initDraft() {
        errors = [:]
        banner = nil
        switch session.family {
        case .uds100:
            draft.report = String(snapshot.reportPeriodMin ?? 20)
            draft.gps = String(snapshot.gpsPeriodMin ?? 60)
            draft.low = String(snapshot.lowThresholdMM ?? 400)
            draft.high = String(snapshot.highThresholdMM ?? 3000)
        case .dc200Family:
            draft.report = String(snapshot.reportPeriodMin ?? 20)
            draft.modeIndex = snapshot.parkMode ?? 0
        case .svc100:
            draft.vol = snapshot.voltageLevel ?? 2
            draft.port = snapshot.portFunction ?? 1
            draft.stable = String(snapshot.stableTimeS ?? 5)
            draft.smart = snapshot.smartPower == 1
            draft.tz = snapshot.timezone ?? 0
            draft.period = String(snapshot.reportPeriodMin ?? 60)
        }
    }

    // MARK: - 表单（按家族）

    @ViewBuilder
    private var formBody: some View {
        if isUDS {
            NumFieldCard(label: zh ? "上报周期" : "Report period",
                         text: $draft.report, unit: zh ? "分钟" : "min",
                         error: errors["report"])
            NumFieldCard(label: zh ? "GPS 周期" : "GPS period",
                         text: $draft.gps, unit: zh ? "分钟" : "min",
                         hint: zh ? "0 = 关闭 GPS 定位" : "0 = GPS positioning off",
                         error: errors["gps"])
            NumFieldCard(label: zh ? "低阈值" : "Low threshold",
                         text: $draft.low, unit: "mm",
                         error: errors["low"])
            NumFieldCard(label: zh ? "高阈值" : "High threshold",
                         text: $draft.high, unit: "mm",
                         hint: zh ? "0 = 关闭高阈值告警" : "0 = high-threshold alarm off",
                         error: errors["high"])
            Text(zh ? "固件校验任一参数非法时整包静默拒绝（无 ACK），App 侧已前置同规则校验"
                    : "Firmware silently rejects the whole payload if any value is invalid; the app pre-validates with the same rules")
                .font(.hkt(11))
                .foregroundStyle(Theme.text2)
                .padding(.top, 4)
        } else if session.family == .dc200Family {
            NumFieldCard(label: zh ? "上报周期" : "Report period",
                         text: $draft.report, unit: zh ? "分钟" : "min",
                         hint: zh ? "0 为固件接受的合法值（开放策略见需求 §7.4）" : "0 is accepted by firmware",
                         error: errors["report"])
            ChoiceChipRow(label: zh ? "工作模式" : "Work mode",
                          options: [zh ? "融合模式" : "Fusion",
                                    zh ? "仅地磁" : "Mag only",
                                    zh ? "雷达优先" : "Radar first"],
                          selection: $draft.modeIndex)
        } else {
            svcForm
        }
    }

    // MARK: - SVC100 表单（两个 cfg-section）

    private var svcForm: some View {
        let stableOn = draft.port & 0x80 != 0
        let rawPort = String(format: "0x%02X", draft.port & 0xff)
        return VStack(alignment: .leading, spacing: 0) {
            ConfigSection(title: zh ? "输出与端口" : "Output & port",
                          caption: zh ? "选择供电档位与两路端口动作" : "Choose supply level and both port actions",
                          state: rawPort) {
                ConfigLabel(text: zh ? "输出电压档" : "Voltage level")
                HStack(spacing: 7) {
                    ForEach(0..<3, id: \.self) { index in
                        ChoiceCell(text: ["12V", "9V", "5V"][index], selected: draft.vol == index) {
                            draft.vol = index
                        }
                    }
                }
                ConfigCaption(text: zh ? "固件映射 0=12V / 1=9V / 2=5V" : "firmware: 0=12V / 1=9V / 2=5V")
                ConfigLabel(text: zh ? "端口功能" : "Port function")
                HStack(alignment: .top, spacing: 7) {
                    PortSet(title: zh ? "阀 1" : "Valve 1",
                            onText: zh ? "开关控制" : "Switch control",
                            offText: zh ? "PWM 控制" : "PWM control",
                            isOn: draft.port & 0x01 != 0) { on in
                        draft.port = derivedPort(draft.port, bit: 0x01, on: on)
                    }
                    PortSet(title: zh ? "阀 2" : "Valve 2",
                            onText: zh ? "开关控制" : "Switch control",
                            offText: zh ? "PWM 控制" : "PWM control",
                            isOn: draft.port & 0x02 != 0) { on in
                        draft.port = derivedPort(draft.port, bit: 0x02, on: on)
                    }
                }
                ConfigCaption(text: zh ? "合成端口值: \(rawPort)" : "Derived port value: \(rawPort)", topSpacing: 7)
                HStack(alignment: .top, spacing: 7) {
                    ConfigControlRow(label: zh ? "含稳定时长" : "Stable time on") {
                        Button { draft.port ^= 0x80 } label: {
                            HKTToggle(isOn: stableOn)
                        }
                    }
                    ConfigInputRow(label: zh ? "稳定时长" : "Stable time",
                                   text: $draft.stable, unit: "s", disabled: !stableOn)
                }
                .padding(.top, 10)
                if let error = errors["stable"] {
                    Text("✕ " + error).font(.hkt(12)).foregroundStyle(Theme.err)
                        .padding(.top, 5)
                } else {
                    ConfigCaption(text: stableOn ? (zh ? "1–255 秒" : "1–255 s")
                                                 : (zh ? "仅在带稳定时间的端口模式下可编辑" : "Only editable in stable-time port modes"),
                                  topSpacing: 5)
                }
                ConfigControlRow(label: zh ? "自动开关机" : "Auto power") {
                    Button { draft.smart.toggle() } label: {
                        HKTToggle(isOn: draft.smart)
                    }
                }
                .padding(.top, 10)
                ConfigCaption(text: zh ? "开启后：阀插入自动开机，阀拔出自动关机" : "When on: the valve powers on when inserted and off when removed",
                              topSpacing: 7)
            }

            ConfigSection(title: zh ? "时间与上报" : "Time & reporting",
                          caption: zh ? "用于 0x06 对时换算与定时任务触发" : "Used for 0x06 time sync and schedule timing",
                          state: tzLabel(draft.tz)) {
                ConfigLabel(text: zh ? "时区" : "Time zone")
                timezonePicker
                if let deviceTz = snapshot.timezone {
                    ConfigCaption(text: (zh ? "设备当前: " : "Device current: ") + tzLabel(deviceTz), topSpacing: 6)
                }
                Button {
                    matchPhoneTimezone()
                } label: {
                    Text(zh ? "与手机时区一致" : "Match phone timezone")
                        .font(.hkt(16, .semibold))
                        .foregroundStyle(Theme.text)
                        .frame(maxWidth: .infinity)
                        .padding(13)
                        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
                        .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius)
                            .stroke(Theme.line, lineWidth: 1))
                }
                .padding(.top, 9)
                if let tzHint {
                    Text(tzHint).font(.hkt(11)).foregroundStyle(Theme.text2)
                        .padding(.top, 4)
                }
                ConfigLabel(text: zh ? "上报周期" : "Report period")
                ConfigInputRow(label: zh ? "上报周期" : "Report period",
                               text: $draft.period, unit: zh ? "分钟" : "min")
                if let error = errors["period"] {
                    Text("✕ " + error).font(.hkt(12)).foregroundStyle(Theme.err)
                        .padding(.top, 5)
                } else {
                    ConfigCaption(text: zh ? "0 为固件接受的合法值（开放策略见需求 §7.4）" : "0 is accepted by firmware",
                                  topSpacing: 5)
                }
            }
        }
    }

    /// 时区分组选择（原型 tzGroups：西半球 / UTC+00:00 / 东半球 / 半小时时区）。
    private var timezonePicker: some View {
        Picker(zh ? "时区" : "Time zone", selection: $draft.tz) {
            Section(zh ? "西半球时区（UTC−12:00 ~ UTC−01:00）" : "Western (UTC−12:00 ~ UTC−01:00)") {
                ForEach((13...24).reversed(), id: \.self) { value in
                    Text(tzLabel(value)).tag(value)
                }
            }
            Section("UTC+00:00") {
                Text(tzLabel(0)).tag(0)
            }
            Section(zh ? "东半球时区（UTC+01:00 ~ UTC+12:00）" : "Eastern (UTC+01:00 ~ UTC+12:00)") {
                ForEach(1...12, id: \.self) { value in
                    Text(tzLabel(value)).tag(value)
                }
            }
            Section(zh ? "半小时时区" : "Half-hour time zones") {
                Text(tzLabel(25)).tag(25)
                Text(tzLabel(26)).tag(26)
            }
        }
        .pickerStyle(.menu)
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(.horizontal, 10).padding(.vertical, 6)
        .background(Theme.card2, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius)
            .stroke(Theme.line, lineWidth: 1))
        .tint(Theme.text)
    }

    private func matchPhoneTimezone() {
        let seconds = TimeZone.current.secondsFromGMT()
        let hours = Double(seconds) / 3600.0
        let half = (hours * 2).rounded()
        var encoded: Int?
        if abs(hours * 2 - half) < 0.01 {
            if half.truncatingRemainder(dividingBy: 2) == 0 {
                let hour = Int(half / 2)
                if hour >= 0 && hour <= 12 { encoded = hour }
                else if hour >= -12 && hour < 0 { encoded = 12 - hour }
            } else if half == 7 { encoded = 25 }
            else if half == 11 { encoded = 26 }
        }
        if let encoded {
            draft.tz = encoded
            tzHint = zh ? "已选为手机时区" : "Set to phone timezone"
        } else {
            draft.tz = 0
            tzHint = zh ? "手机时区超出设备支持范围，已选 UTC+00:00" : "Phone timezone unsupported; set to UTC+00:00"
        }
    }

    /// 位运算合成（原型 svcDerivedPort）：`base=(port&3&~bit)|(mode?bit:0)` 再并 `port&0x80`。
    private func derivedPort(_ port: Int, bit: Int, on: Bool) -> Int {
        let base = (port & 3 & ~bit) | (on ? bit : 0)
        return base | (port & 0x80)
    }

    private func tzLabel(_ value: Int) -> String {
        if value == 25 { return "UTC+03:30" }
        if value == 26 { return "UTC+05:30" }
        return value < 13
            ? String(format: "UTC+%02d:00", value)
            : String(format: "UTC−%02d:00", value - 12)
    }

    // MARK: - 校验（cfgValidate：与固件同规则，任一非法拦截保存）

    private var validationErrors: [String: String] {
        var result: [String: String] = [:]
        let outOfRange: (String, Int, Int) -> Void = { key, minV, maxV in
            result[key] = zh ? "超出范围 \(minV)–\(maxV)" : "Out of range \(minV)–\(maxV)"
        }
        let number = { (text: String) -> Int? in Int(text.trimmingCharacters(in: .whitespaces)) }
        if isUDS {
            if let value = number(draft.report), (1...1440).contains(value) {} else {
                outOfRange("report", 1, 1440)
            }
            if let value = number(draft.gps), value == 0 || (10...1440).contains(value) {} else {
                result["gps"] = zh ? "合法值：0 或 10–1440" : "valid: 0 or 10–1440"
            }
            if let value = number(draft.low), (30...4500).contains(value) {} else {
                outOfRange("low", 30, 4500)
            }
            if let value = number(draft.high), value == 0 || (30...4500).contains(value) {} else {
                result["high"] = zh ? "合法值：0 或 30–4500" : "valid: 0 or 30–4500"
            }
        } else if session.family == .dc200Family {
            if let value = number(draft.report), (0...1440).contains(value) {} else {
                outOfRange("report", 0, 1440)
            }
        } else {
            if draft.port & 0x80 != 0 {
                if let value = number(draft.stable), (1...255).contains(value) {} else {
                    outOfRange("stable", 1, 255)
                }
            }
            if let value = number(draft.period), (0...1440).contains(value) {} else {
                outOfRange("period", 0, 1440)
            }
        }
        return result
    }

    // MARK: - 保存流（校验 → 确认摘要 → 写入中 → 结果横幅）

    private var saveArea: some View {
        VStack(spacing: 8) {
            saveButton
            Text(payloadText)
                .font(.hkt(11))
                .foregroundStyle(Theme.text2)
        }
        .padding(.top, 12)
    }

    private var saveButton: some View {
        Button {
            saveTapped()
        } label: {
            Text(zh ? "保存配置" : "Save")
                .font(.hkt(16, .semibold))
                .foregroundStyle(.white)
                .frame(maxWidth: .infinity)
                .padding(13)
                .background(Theme.info, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
        }
    }

    private var payloadText: String {
        let size = isUDS ? 9 : (session.family == .dc200Family ? 4 : 8)
        return "0x02 · " + (zh ? "载荷" : "payload") + " \(size) B"
    }

    private func saveTapped() {
        let found = validationErrors
        errors = found
        guard found.isEmpty else { return }   // 行内已显示错误，拦截保存
        showConfirm = true
    }

    /// 确认框摘要表（cfgSummary：label text2 + 值粗体）。
    @ViewBuilder
    private var summaryRows: some View {
        let minutes = zh ? "分钟" : "min"
        switch session.family {
        case .uds100:
            summaryRow(zh ? "上报周期" : "Report period", "\(draft.report) \(minutes)")
            summaryRow(zh ? "GPS 周期" : "GPS period", "\(draft.gps) \(minutes)")
            summaryRow(zh ? "低阈值" : "Low threshold", "\(draft.low) mm")
            summaryRow(zh ? "高阈值" : "High threshold", "\(draft.high) mm")
        case .dc200Family:
            summaryRow(zh ? "上报周期" : "Report period", "\(draft.report) \(minutes)")
            summaryRow(zh ? "工作模式" : "Work mode", [zh ? "融合模式" : "Fusion",
                                                       zh ? "仅地磁" : "Mag only",
                                                       zh ? "雷达优先" : "Radar first"][draft.modeIndex])
        case .svc100:
            summaryRow(zh ? "输出电压档" : "Voltage level", ["12V", "9V", "5V"][draft.vol])
            summaryRow(zh ? "端口功能" : "Port function", portText(draft.port))
            summaryRow(zh ? "稳定时长" : "Stable time", "\(draft.stable) s")
            summaryRow(zh ? "自动开关机" : "Auto power", draft.smart ? (zh ? "自动" : "Auto") : (zh ? "手动" : "Manual"))
            summaryRow(zh ? "时区" : "Time zone", tzLabel(draft.tz))
            summaryRow(zh ? "上报周期" : "Report period", "\(draft.period) \(minutes)")
        }
    }

    /// 端口 7 值文案（原型 portF 数组）。
    private func portText(_ port: Int) -> String {
        let values = [0x00, 0x01, 0x02, 0x03, 0x81, 0x82, 0x83]
        let labels = zh
            ? ["端口1+2 脉冲", "端口1 开关 · 端口2 脉冲", "端口1 脉冲 · 端口2 开关", "端口1+2 开关",
               "端口1 开关 · 端口2 脉冲（稳定时间）", "端口1 脉冲 · 端口2 开关（稳定时间）", "端口1+2 开关（稳定时间）"]
            : ["Port 1 + Port 2 pulse", "Port 1 switch, Port 2 pulse", "Port 1 pulse, Port 2 switch", "Port 1 + Port 2 switch",
               "Port 1 switch, Port 2 pulse (stable time)", "Port 1 pulse, Port 2 switch (stable time)", "Port 1 + Port 2 switch (stable time)"]
        guard let index = values.firstIndex(of: port) else { return zh ? "未知" : "Unknown" }
        return labels[index]
    }

    private func summaryRow(_ label: String, _ value: String) -> some View {
        HStack(alignment: .top) {
            Text(label)
            Spacer()
            Text(value).bold().foregroundStyle(Theme.text)
        }
        .padding(.vertical, 2)
    }

    @ViewBuilder
    private var confirmDialog: some View {
        if showConfirm {
            DialogScaffold(title: zh ? "写入以下配置？" : "Write these values?") {
                VStack(spacing: 2) { summaryRows }
                    .padding(.vertical, 4)
            } buttons: {
                DialogButton(title: zh ? "取消" : "Cancel") { showConfirm = false }
                DialogButton(title: zh ? "确认写入" : "Write Values", kind: .primary) {
                    showConfirm = false
                    write()
                }
            }
        }
    }

    @ViewBuilder
    private var savingDialog: some View {
        if showSaving {
            DialogScaffold(title: zh ? "正在写入配置…" : "Writing configuration…",
                           centeredBody: true,
                           content: {
                Text("0x02 → \(session.deviceName)")
            }, buttons: { EmptyView() })
        }
    }

    /// cfgWrite：真实 0x02 写入——按家族编码载荷发送，等设备 ACK（含 0xFF 应答段的专用帧）。
    private func write() {
        showSaving = true
        LogStore.shared.info(zh ? "0x02 写入配置 → \(session.deviceName)" : "0x02 write config → \(session.deviceName)")
        let payload: Data
        switch session.family {
        case .uds100:
            payload = HKTFrameEncoder.udsConfigPayload(reportMin: Int(draft.report) ?? 0,
                                                       gpsMin: Int(draft.gps) ?? 0,
                                                       lowMM: Int(draft.low) ?? 0,
                                                       highMM: Int(draft.high) ?? 0)
        case .dc200Family:
            payload = HKTFrameEncoder.dcConfigPayload(reportMin: Int(draft.report) ?? 0,
                                                      mode: draft.modeIndex)
        case .svc100:
            payload = HKTFrameEncoder.svcConfigPayload(volLevel: draft.vol,
                                                       port: draft.port,
                                                       stableS: Int(draft.stable) ?? 0,
                                                       autoPower: draft.smart ? 1 : 0,
                                                       timezone: draft.tz,
                                                       reportMin: Int(draft.period) ?? 0)
        }
        Task {
            let acked = await session.sendWrite(cmd: CommandCode.config, data: payload)
            showSaving = false
            banner = acked ? .ok : .fail
            LogStore.shared.info(acked
                ? (zh ? "0x02 ACK（设备已确认）" : "0x02 ACK (device acknowledged)")
                : (zh ? "0x02 等待确认超时（无 ACK）" : "0x02 ACK timeout"))
        }
    }

    // MARK: - 结果横幅（ok=绿带「完成」/ fail=红带「重试」）

    @ViewBuilder
    private func resultBanner(_ kind: BannerKind) -> some View {
        switch kind {
        case .ok:
            HKTBanner(kind: .ok,
                      text: "✓ " + (zh ? "配置已保存（设备已确认）" : "Configuration saved (device acknowledged)"),
                      actionTitle: zh ? "完成" : "Done",
                      action: { initDraft() })
        case .fail:
            HKTBanner(kind: .err,
                      text: "✕ " + (zh ? "设备未确认配置（可能被固件拒绝），请检查参数范围后重试" : "Device did not acknowledge (possibly rejected). Check ranges and retry."),
                      actionTitle: zh ? "重试" : "Retry",
                      action: { banner = nil })
        }
    }
}
