import CoreBLE
import CoreProtocol
import SwiftUI

/// P-03 设备详情页 —— 1:1 克隆冻结原型（规格卡 `docs/ios/design/ui-spec/P-03.md`）。
/// 结构：会话控制卡（页头，无系统导航栏）→ 滚动区（横幅/状态/设备操作）→ 确认对话框浮层。
/// 数据 = DeviceSession 1s 轮询快照；组件全部来自 DesignSystem，本页不私调样式（AGENTS.md §9）。
struct DeviceDetailView: View {
    let session: DeviceSession
    let onDisconnect: () -> Void

    @Environment(ScanModel.self) private var scanModel
    @Environment(LanguageStore.self) private var langStore
    @Environment(\.dismiss) private var dismiss
    @State private var confirmDisconnect = false
    @State private var confirmPowerOff = false
    @State private var showCalibration = false
    @State private var showConfig = false

    private var snapshot: DeviceSnapshot { session.snapshot }
    private var zh: Bool { langStore.isZh }

    var body: some View {
        // 早退优先级与原型一致：关机 > 断线 > 正常（升级模式随 OTA 里程碑接入）
        if isPowerOff {
            powerOffView
        } else {
            normalChrome
        }
    }

    // MARK: - 页面骨架

    private var normalChrome: some View {
        VStack(spacing: 0) {
            sessionCard
            if session.linkLost {
                // 原型 P_detail disconnected 分支：居中断线视图替换全部 body
                CenterStateView(glyph: "📵", glyphSize: 44,
                                title: zh ? "连接已断开" : "Disconnected",
                                buttonTitle: zh ? "重新连接" : "Retry",
                                action: retryReconnect)
                .padding(.top, 40)
            } else {
                ScrollView {
                    VStack(alignment: .leading, spacing: 0) {
                        if session.unknownTail {
                            HKTBanner(kind: .err,
                                      text: "⚠︎ " + abnormalText,
                                      actionTitle: zh ? "导出日志" : "Export Log",
                                      action: { /* 日志页随诊断里程碑接入 */ })
                        }
                        SectionHeader(title: zh ? "状态（实时轮询）" : "Status (live polling)")
                        statusContent
                        SectionHeader(title: zh ? "设备操作" : "Device actions")
                        opsPanel
                    }
                    .padding(.horizontal, 16)
                    .padding(.bottom, 24)
                }
            }
        }
        .background(Theme.bg)
        .toolbar(.hidden, for: .navigationBar)      // 原型详情页无导航栏，会话卡即页头
        .onAppear {
            // 演示自动导航（-demo-page config）：进入详情后自动进配置页
            let arguments = ProcessInfo.processInfo.arguments
            if arguments.contains("-demo-page config")
                || zip(arguments, arguments.dropFirst()).contains(where: { $0 == "-demo-page" && $1 == "config" }) {
                DispatchQueue.main.asyncAfter(deadline: .now() + 0.8) { showConfig = true }
            }
        }
        .navigationDestination(isPresented: $showCalibration) {
            CalibrationView(family: session.family)
        }
        .navigationDestination(isPresented: $showConfig) {
            ConfigView(session: session)
        }
        .overlay { confirmDialogs }
    }

    /// 原型 !S.powerOn 早退：navbar.small 页头 + 居中「已关机」。
    private var powerOffView: some View {
        VStack(spacing: 0) {
            NavbarHeader(title: session.deviceName,
                         backText: zh ? "‹ 返回" : "‹ Back",
                         onBack: { dismiss() }) {
                LinkButton(title: zh ? "⌂ 首页" : "⌂ Home") { popToRoot() }
            }
            CenterStateView(glyph: "⏻", glyphSize: 46,
                            title: zh ? "已关机" : "Powered off",
                            buttonTitle: zh ? "开机" : "On",
                            action: { sendPower(true) })
        }
        .background(Theme.bg)
        .toolbar(.hidden, for: .navigationBar)
    }

    private func retryReconnect() {
        // R-6 自动重连状态机接入前：驻留原身份直接重建会话
        scanModel.reconnectResident()
    }

    private func popToRoot() {
        NotificationCenter.default.post(name: .init("popToRoot"), object: nil)
    }

    private func sendPower(_ on: Bool) {
        session.send(cmd: CommandCode.power, data: HKTFrameEncoder.powerPayload(on: on))
    }

    // MARK: - 会话控制卡（规格卡 §3.1）

    private var sessionCard: some View {
        SessionControlCard(deviceName: session.deviceName,
                           meta: metaText,
                           badge: headBadge) {
            SessionButton(title: zh ? "返回" : "Back") { dismiss() }
            SessionButton(title: zh ? "首页" : "Home") { popToRoot() }
            SessionButton(title: zh ? "固件升级" : "Firmware Update") { /* P-04 OTA 里程碑接入 */ }
            SessionButton(title: zh ? "断开连接" : "Disconnect", danger: true) { confirmDisconnect = true }
        }
        .padding(.top, 8)
    }

    private var metaText: String {
        let fw = "v\(snapshot.hardwareVersion).\(snapshot.softwareVersion)"
        let batt: String
        if session.family == .uds100, let mv = snapshot.batteryVoltageMV {
            batt = (zh ? "电压 " : "Voltage ") + "\(mv) mV"
        } else if let pct = snapshot.batteryPercent {
            batt = (zh ? "电量 " : "Battery ") + "\(pct)%"
        } else {
            batt = "—"
        }
        return (zh ? "固件 " : "Firmware ") + fw + " · " + batt
    }

    /// 状态徽章：ready=ok / stale=warn / abnormal=err（原型 head 逻辑）。
    private var headBadge: StateBadge? {
        if session.unknownTail {
            return StateBadge(kind: .err, text: abnormalText)
        }
        if session.isStale {
            let s = session.secondsSinceLastResponse ?? 0
            return StateBadge(kind: .warn,
                              text: zh ? "最后更新 \(s) 秒前 · 正在重试" : "Updated \(s)s ago · retrying")
        }
        return StateBadge(kind: .ok, text: zh ? "已连接 · 轮询正常" : "Connected · polling")
    }

    private var abnormalText: String {
        zh ? "响应数据异常（未知类型），已显示可解析字段"
           : "Response abnormal (unknown type); parsed fields shown"
    }

    // MARK: - 状态区（规格卡 §4：字段顺序=原型 defs，不可重排）

    @ViewBuilder
    private var statusContent: some View {
        switch session.family {
        case .svc100:
            SvcChannelModule(title: zh ? "阀状态" : "Valve state",
                             tag: isStableMode ? (zh ? "含稳定时长" : "Stable time on")
                                               : (zh ? "标准模式" : "Standard mode"),
                             channels: svcChannels)
                .padding(.bottom, 8)
            FieldGrid(fields: tailUnknown(svcFields))
        case .uds100:
            FieldGrid(fields: tailUnknown(udsFields))
        case .dc200Family:
            FieldGrid(fields: tailUnknown(dcFields))
        }
    }

    /// 原型 unknownTail 规则：异常态最后一个字段值显示 "…"。
    private func tailUnknown(_ fields: [FieldSpec]) -> [FieldSpec] {
        guard session.unknownTail, var last = fields.last else { return fields }
        last = FieldSpec(last.label, "…", unit: nil)
        return fields.dropLast() + [last]
    }

    private var dcFields: [FieldSpec] {
        [
            FieldSpec(zh ? "车位状态" : "Parking", parkText),
            FieldSpec(zh ? "工作模式" : "Work mode", modeText),
            FieldSpec(zh ? "防拆状态" : "Tamper", triggeredText(snapshot.tamper)),
            FieldSpec(zh ? "上报周期" : "Report period", minutesEmbedded(snapshot.reportPeriodMin)),
        ]
    }

    private var udsFields: [FieldSpec] {
        [
            FieldSpec(zh ? "温度" : "Temperature", milli3(snapshot.temperatureMilli), unit: "°C"),
            FieldSpec(zh ? "湿度" : "Humidity", milli3(snapshot.humidityMilli), unit: "%"),
            FieldSpec(zh ? "距离" : "Distance", of(snapshot.distanceMM), unit: "mm"),
            FieldSpec(zh ? "满溢状态" : "Overflow", overflowText),
            FieldSpec(zh ? "低阈值" : "Low threshold", of(snapshot.lowThresholdMM), unit: "mm"),
            FieldSpec(zh ? "高阈值" : "High threshold", of(snapshot.highThresholdMM), unit: "mm"),
            FieldSpec(zh ? "倾角" : "Tilt angle", centi2(snapshot.angleCenti), unit: "°"),
            FieldSpec(zh ? "倾斜" : "Slant", flagText(snapshot.slant)),
            FieldSpec(zh ? "温湿告警" : "HT alarm", triggeredText(snapshot.htAlarm)),
            FieldSpec(zh ? "GPS 周期" : "GPS period", minutesEmbedded(snapshot.gpsPeriodMin)),
            FieldSpec(zh ? "上报周期" : "Report period", minutesEmbedded(snapshot.reportPeriodMin)),
            FieldSpec(zh ? "纬度" : "Latitude", fixed4(snapshot.latitude)),
            FieldSpec(zh ? "经度" : "Longitude", fixed4(snapshot.longitude)),
        ]
    }

    private var svcFields: [FieldSpec] {
        [
            FieldSpec(zh ? "输出电压档" : "Voltage level", voltText),
            FieldSpec(zh ? "稳定时长" : "Stable time", of(snapshot.stableTimeS), unit: "s"),
            FieldSpec(zh ? "自动开关机" : "Auto power", smartText),
            FieldSpec(zh ? "时区" : "Time zone", tzText),
            FieldSpec(zh ? "上报周期" : "Report period", of(snapshot.reportPeriodMin), unit: zh ? "分钟" : "min"),
        ]
    }

    private var svcChannels: [SvcChannelSpec] {
        let mode = (snapshot.portFunction ?? 0) & 0x03
        let yes = zh ? "是" : "Yes", no = zh ? "否" : "No"
        let onT = zh ? "开启" : "On", offT = zh ? "关闭" : "Off"
        let insertLabel = zh ? "插入检测" : "Insert detect"
        let pulseLabel = zh ? "脉冲计数" : "Pulse count"
        let portLabel = zh ? "端口功能" : "Port function"
        let port = zh ? "开关控制" : "Switch control"
        let pwm = zh ? "PWM 控制" : "PWM control"

        func channel(_ name: String, _ on: Int?, _ inserted: Int?, _ pulse: Int?, switchMode: Bool) -> SvcChannelSpec {
            SvcChannelSpec(name: name,
                           on: on == 1,
                           onText: on == 1 ? onT : offT,
                           insertLabel: insertLabel,
                           insertText: inserted == 1 ? yes : no,
                           pulseLabel: pulseLabel,
                           pulseText: (pulse ?? 0).formatted(),
                           portLabel: portLabel,
                           portText: switchMode ? port : pwm)
        }
        return [
            channel(zh ? "阀 1" : "V1", snapshot.valve1State, snapshot.valve1Inserted, snapshot.valve1Pulse,
                    switchMode: mode == 1 || mode == 3),
            channel(zh ? "阀 2" : "V2", snapshot.valve2State, snapshot.valve2Inserted, snapshot.valve2Pulse,
                    switchMode: mode == 2 || mode == 3),
        ]
    }

    /// 稳定时长模式位：port & 0x80（原型 stableModeOn）。
    private var isStableMode: Bool { (snapshot.portFunction ?? 0) & 0x80 != 0 }

    // MARK: - 设备操作面板（规格卡 §3.6）

    private var opsPanel: some View {
        OpsPanel(powerRow: OpsPowerRow(label: zh ? "电源" : "Power",
                                       stateText: snapshot.power == 1 ? (zh ? "开启" : "On") : (zh ? "已关机" : "Off"),
                                       isOn: snapshot.power == 1,
                                       onToggle: togglePower)) {
            if session.family != .svc100 {
                OpCard(badge: "CAL", badgeKind: .warn,
                       title: session.family == .uds100 ? (zh ? "角度校准" : "Tilt Calibration")
                                                        : (zh ? "磁力计校准" : "Mag Calibration"),
                       desc: zh ? "环境检查 · 长时操作" : "Environment checks · long-running") {
                    showCalibration = true
                }
            }
            if session.family == .dc200Family {
                OpCard(badge: "MAG",
                       title: zh ? "技术参数" : "Tech Parameters",
                       desc: zh ? "三轴曲线 · 雷达频谱" : "3-axis curves · radar spectrum") {}
            }
            OpCard(badge: "CFG",
                   title: zh ? "参数配置" : "Configuration",
                   desc: zh ? "上报 / 端口 / 时区" : "Reporting / port / timezone") {
                showConfig = true
            }
            TimelineView(.periodic(from: .now, by: 1)) { context in
                OpCard(badge: "TIME", badgeKind: .ok,
                       title: zh ? "时间同步" : "Time Sync",
                       desc: clockText(context.date),
                       trailing: .pill(session.isTimeSyncing ? (zh ? "同步中" : "Syncing")
                                       : session.timeSyncDone ? (zh ? "完成" : "Done")
                                       : (zh ? "同步" : "Sync")),
                       disabled: session.isTimeSyncing) {
                    session.sendTimeSync()
                }
            }
        }
    }

    private func togglePower() {
        if snapshot.power == 1 { confirmPowerOff = true } else { sendPower(true) }
    }

    // MARK: - 确认对话框（规格卡 §3.7，原型 .dialog 296/r18）

    @ViewBuilder
    private var confirmDialogs: some View {
        if confirmPowerOff {
            HKTDialog(title: zh ? "确认关机？" : "Power off?",
                      message: zh ? "关机后设备停止上报与工作，之后可再次开机。"
                                  : "The device stops reporting and working. You can power it on again.") {
                DialogButton(title: zh ? "取消" : "Cancel") { confirmPowerOff = false }
                DialogButton(title: zh ? "确认关机" : "Power Off", kind: .danger) {
                    confirmPowerOff = false
                    sendPower(false)
                }
            }
        }
        if confirmDisconnect {
            HKTDialog(title: zh ? "确认断开连接？" : "Disconnect?",
                      message: zh ? "断开后将停止实时轮询并断开 BLE；这是预期断开，不会自动重连。设备会进入最近设备。"
                                  : "Disconnecting stops live polling and disconnects BLE. This is an expected disconnect and will not auto-reconnect. The device will be kept in Recents.") {
                DialogButton(title: zh ? "取消" : "Cancel") { confirmDisconnect = false }
                DialogButton(title: zh ? "确认断开" : "Disconnect", kind: .danger) {
                    confirmDisconnect = false
                    session.stop()
                    scanModel.residentDevice = nil
                    onDisconnect()
                }
            }
        }
    }

    // MARK: - 取值格式化（=原型 fval，逐条对齐规格卡 §4）

    private var isPowerOff: Bool { snapshot.power == 0 }

    private var overflowText: String {
        switch snapshot.overflowState {
        case 0: return zh ? "正常" : "Normal"
        case 1: return zh ? "低阈值触发" : "Low threshold triggered"
        case 2: return zh ? "高阈值触发" : "High threshold triggered"
        case 0xFF: return zh ? "无效" : "Invalid"
        case .some: return zh ? "未知" : "Unknown"
        case nil: return "—"
        }
    }

    private var parkText: String {
        switch snapshot.parkState {
        case 0: return zh ? "空位" : "Vacant"
        case 1: return zh ? "有车" : "Occupied"
        case 0xFF: return zh ? "被遮挡" : "Covered"
        case .some: return zh ? "未知" : "Unknown"
        case nil: return "—"
        }
    }

    private var modeText: String {
        switch snapshot.parkMode {
        case 0: return zh ? "融合模式" : "Fusion"
        case 1: return zh ? "仅地磁" : "Mag only"
        case 2: return zh ? "雷达优先" : "Radar first"
        case .some: return zh ? "未知" : "Unknown"
        case nil: return "—"
        }
    }

    private var voltText: String {
        switch snapshot.voltageLevel {
        case 0: return "12V"
        case 1: return "9V"
        case 2: return "5V"
        case .some: return "?"
        case nil: return "—"
        }
    }

    private var smartText: String {
        switch snapshot.smartPower {
        case 1: return zh ? "自动" : "Auto"
        case 0: return zh ? "手动" : "Manual"
        default: return "—"
        }
    }

    /// 原型：25=UTC+03:30，26=UTC+05:30，<13=UTC+{n}，否则=UTC−{n-12}（无 :00 后缀）。
    private var tzText: String {
        switch snapshot.timezone {
        case 25: return "UTC+03:30"
        case 26: return "UTC+05:30"
        case .some(let v) where v < 13: return "UTC+\(v)"
        case .some(let v) where v <= 24: return "UTC−\(v - 12)"
        default: return "—"
        }
    }

    private func triggeredText(_ value: Int?) -> String {
        switch value {
        case 1: return zh ? "已触发" : "Triggered"
        case 0: return zh ? "正常" : "Normal"
        default: return zh ? "未知" : "Unknown"
        }
    }

    private func flagText(_ value: Int?) -> String {
        switch value {
        case 1: return zh ? "是" : "Yes"
        case 0: return zh ? "否" : "No"
        default: return zh ? "未知" : "Unknown"
        }
    }

    /// `{n} 分钟`：DC/UDS 的周期字段单位嵌在值文本里（原型 fval f_period/f_gps）。
    private func minutesEmbedded(_ value: Int?) -> String {
        guard let value else { return "—" }
        return "\(value) \(zh ? "分钟" : "min")"
    }

    private func milli3(_ value: Int?) -> String { value.map { String(format: "%.3f", Double($0) / 1000) } ?? "—" }
    private func centi2(_ value: Int?) -> String { value.map { String(format: "%.2f", Double($0) / 100) } ?? "—" }
    private func fixed4(_ value: Double?) -> String { value.map { String(format: "%.4f", $0) } ?? "—" }
    private func of(_ value: Int?) -> String { value.map(String.init) ?? "—" }

    private static let clockFormatter: DateFormatter = {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss"    // 原型 liveClockStr 固定格式
        return formatter
    }()

    private func clockText(_ date: Date) -> String {
        Self.clockFormatter.string(from: date)
    }
}
