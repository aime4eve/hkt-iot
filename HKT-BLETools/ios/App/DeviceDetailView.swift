import CoreBLE
import CoreProtocol
import SwiftUI

/// P-03 设备详情页（1:1 克隆已确认原型；数据 = DeviceSession 1s 轮询快照）。
struct DeviceDetailView: View {
    let session: DeviceSession
    let onDisconnect: () -> Void

    @Environment(ScanModel.self) private var scanModel
    @State private var confirmDisconnect = false
    @State private var confirmPowerOff = false

    private var snapshot: DeviceSnapshot { session.snapshot }
    private var zh: Bool { Locale.current.language.languageCode?.identifier == "zh" }

    var body: some View {
        VStack(spacing: 0) {
            sessionCard
            if session.isStale {
                banner(staleBannerText, color: Theme.warn)
            }
            if session.unknownTail {
                banner(zh ? "⚠ 响应数据异常（未知类型），已显示可解析字段"
                         : "⚠ Abnormal response (unknown type); parsed fields shown", color: Theme.err)
            }
            ScrollView {
                VStack(alignment: .leading, spacing: 0) {
                    sectionHeader(zh ? "状态（实时轮询）" : "Status (live polling)")
                    statusGrid
                    sectionHeader(zh ? "设备操作" : "Device actions")
                    opsPanel
                }
                .padding(.horizontal, 16)
                .padding(.bottom, 24)
            }
        }
        .background(Theme.bg)
        .navigationTitle(session.deviceName)
        .navigationBarTitleDisplayMode(.inline)
        .toolbar {
            ToolbarItem(placement: .topBarTrailing) {
                Button(zh ? "首页" : "Home") { NotificationCenter.default.post(name: .init("popToRoot"), object: nil) }
            }
        }
        .alert(zh ? "确认断开连接？" : "Disconnect?", isPresented: $confirmDisconnect) {
            Button(zh ? "断开" : "Disconnect", role: .destructive) { disconnect() }
            Button(zh ? "取消" : "Cancel", role: .cancel) {}
        } message: {
            Text(zh ? "断开后将停止实时轮询并断开 BLE；这是预期断开，不会自动重连。设备会进入最近设备。"
                    : "Stops live polling and disconnects BLE. Expected disconnect, no auto-reconnect. Device stays in Recents.")
        }
        .alert(zh ? "确认关机？" : "Power off?", isPresented: $confirmPowerOff) {
            Button(zh ? "关机" : "Power Off", role: .destructive) {
                session.send(cmd: CommandCode.power, data: HKTFrameEncoder.powerPayload(on: false))
            }
            Button(zh ? "取消" : "Cancel", role: .cancel) {}
        } message: {
            Text(zh ? "关机后设备停止上报与工作，之后可再次开机。" : "Device stops reporting; you can power it on again.")
        }
    }

    private func disconnect() {
        session.stop()
        scanModel.residentDevice = nil
        onDisconnect()
    }

    private func dismissToRoot() {
        NotificationCenter.default.post(name: .init("popToRoot"), object: nil)
    }

    // MARK: - 会话控制卡（SP-G6）

    private var sessionCard: some View {
        VStack(alignment: .leading, spacing: 6) {
            HStack {
                Text(session.deviceName).font(.title2).fontWeight(.bold)
                Spacer()
                if !session.isStale {
                    Text(zh ? "已连接 · 轮询正常" : "Connected · polling")
                        .font(.caption2).fontWeight(.bold)
                        .padding(.horizontal, 8).padding(.vertical, 3)
                        .background(Theme.ok.opacity(0.15), in: Capsule())
                        .foregroundStyle(Theme.ok)
                }
            }
            Text("\(zh ? "固件" : "Firmware") v\(snapshot.hardwareVersion).\(snapshot.softwareVersion) · \(metaPower)")
                .font(.caption).foregroundStyle(Theme.text2)
            HStack(spacing: 6) {
                sessionBtn(zh ? "返回" : "Back") { NotificationCenter.default.post(name: .init("popToRoot"), object: nil) }
                sessionBtn(zh ? "首页" : "Home") { dismissToRoot() }
                sessionBtn(zh ? "固件升级" : "Firmware Update") { /* P-04 后续里程碑 */ }
                sessionBtn(zh ? "断开连接" : "Disconnect", danger: true) { confirmDisconnect = true }
            }
        }
        .padding(14)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .padding(.horizontal, 16).padding(.top, 8)
    }

    private var metaPower: String {
        if let mv = snapshot.batteryVoltageMV { return "\(mv) mV" }
        if let pct = snapshot.batteryPercent { return zh ? "电量 \(pct)%" : "Battery \(pct)%" }
        return "—"
    }

    private func sessionBtn(_ title: String, danger: Bool = false, action: @escaping () -> Void) -> some View {
        Button(title, action: action)
            .font(.caption).fontWeight(.semibold)
            .frame(maxWidth: .infinity).frame(height: 34)
            .background(danger ? Theme.err.opacity(0.08) : Theme.card2, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
            .foregroundStyle(danger ? Theme.err : Theme.text)
            .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius).stroke(Theme.line, lineWidth: 1))
    }

    private var staleBannerText: String {
        let seconds = session.secondsSinceLastResponse ?? 0
        return zh ? "最后更新 \(seconds) 秒前 · 正在重试" : "Updated \(seconds)s ago · retrying"
    }

    // MARK: - 状态字段区（SP-5 字段全集）

    @ViewBuilder
    private var statusGrid: some View {
        LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())], spacing: 8) {
            switch snapshot.family {
            case .uds100:
                field(zh ? "温度" : "Temperature", tempC, "°C")
                field(zh ? "湿度" : "Humidity", humPercent, "%")
                field(zh ? "距离" : "Distance", of(snapshot.distanceMM), "mm")
                field(zh ? "满溢状态" : "Overflow", overflowText)
                field(zh ? "低阈值" : "Low threshold", of(snapshot.lowThresholdMM), "mm")
                field(zh ? "高阈值" : "High threshold", of(snapshot.highThresholdMM), "mm")
                field(zh ? "倾角" : "Tilt", angleDeg, "°")
                field(zh ? "倾斜" : "Slant", flag(snapshot.slant, yes: zh ? "是" : "Yes", no: zh ? "否" : "No"))
                field(zh ? "温湿告警" : "HT alarm", alarmText)
                field(zh ? "GPS 周期" : "GPS period", of(snapshot.gpsPeriodMin), "min")
                field(zh ? "上报周期" : "Report period", of(snapshot.reportPeriodMin), "min")
                field(zh ? "纬度" : "Latitude", of(snapshot.latitude))
                field(zh ? "经度" : "Longitude", of(snapshot.longitude))
            case .dc200Family:
                field(zh ? "车位状态" : "Parking", parkText)
                field(zh ? "工作模式" : "Work mode", modeText)
                field(zh ? "防拆状态" : "Tamper", alarmStyle(snapshot.tamper))
                field(zh ? "上报周期" : "Report period", of(snapshot.reportPeriodMin), "min")
            case .svc100:
                field(zh ? "输出电压档" : "Voltage level", voltText)
                field(zh ? "端口功能" : "Port function", portText)
                field(zh ? "稳定时长" : "Stable time", of(snapshot.stableTimeS), "s")
                field(zh ? "自动开关机" : "Auto power", smartText)
                field(zh ? "时区" : "Time zone", tzText)
                field(zh ? "上报周期" : "Report period", of(snapshot.reportPeriodMin), "min")
            }
        }
    }

    // MARK: - 设备操作面板（SP-G6）

    private var opsPanel: some View {
        VStack(spacing: 0) {
            powerRow
            VStack(spacing: 8) {
                if session.family != .svc100 {
                    opTile("CAL",
                           session.family == .uds100 ? (zh ? "倾角校准" : "Tilt Calibration") : (zh ? "磁力计校准" : "Mag Calibration"),
                           zh ? "环境检查 · 长时操作" : "Environment checks · long-running") {}
                }
                if session.family == .dc200Family {
                    opTile("MAG", zh ? "技术参数" : "Tech Parameters",
                           zh ? "三轴曲线 · 雷达频谱" : "3-axis curves · radar spectrum") {}
                }
                opTile("CFG", zh ? "参数配置" : "Configuration",
                       zh ? "上报 / 端口 / 时区" : "Reporting / port / timezone") {}
                opTile("TIME", zh ? "时间同步" : "Time Sync",
                       liveClock, trailing: syncText, disabled: session.isTimeSyncing) {
                    session.sendTimeSync()
                }
            }
            .padding(10)
        }
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius).stroke(Theme.line, lineWidth: 1))
    }

    private var powerRow: some View {
        Button {
            if snapshot.power == 1 { confirmPowerOff = true } else {
                session.send(cmd: CommandCode.power, data: HKTFrameEncoder.powerPayload(on: true))
            }
        } label: {
            HStack {
                Text(zh ? "电源" : "Power").font(.subheadline).fontWeight(.semibold)
                Text(snapshot.power == 1 ? (zh ? "开启" : "On") : (zh ? "已关机" : "Off"))
                    .font(.caption).foregroundStyle(Theme.text2)
                Spacer()
                Capsule().fill(snapshot.power == 1 ? Theme.ok : Theme.fill).frame(width: 44, height: 26)
                    .overlay(Circle().fill(.white).frame(width: 22, height: 22).offset(x: snapshot.power == 1 ? 9 : -9))
            }
            .padding(.horizontal, 14)
        }
        .frame(minHeight: 58)
    }

    private func opTile(_ badge: String, _ title: String, _ desc: String,
                        trailing: String = "›", disabled: Bool = false,
                        action: @escaping () -> Void) -> some View {
        Button(action: action) {
            HStack(spacing: 10) {
                Text(badge).font(.caption2).fontWeight(.bold)
                    .frame(width: 38, height: 38)
                    .background(Theme.info.opacity(0.1), in: RoundedRectangle(cornerRadius: 10))
                    .foregroundStyle(Theme.info)
                VStack(alignment: .leading, spacing: 3) {
                    Text(title).font(.subheadline).fontWeight(.semibold)
                    Text(desc).font(.caption2).foregroundStyle(Theme.text2)
                }
                Spacer()
                Text(trailing).font(.caption).fontWeight(.bold).foregroundStyle(Theme.info)
            }
        }
        .disabled(disabled)
        .opacity(disabled ? 0.45 : 1)
        .background(Theme.card2, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius).stroke(Theme.line, lineWidth: 1))
    }

    // MARK: - 通用小块

    private func sectionHeader(_ title: String) -> some View {
        Text(title).font(.footnote).fontWeight(.semibold).foregroundStyle(Theme.text2)
            .padding(.top, 12).padding(.bottom, 6)
    }

    private func banner(_ text: String, color: Color) -> some View {
        Text(text).font(.caption).fontWeight(.semibold)
            .frame(maxWidth: .infinity, alignment: .leading)
            .padding(10)
            .background(color.opacity(0.12), in: RoundedRectangle(cornerRadius: 8))
            .foregroundStyle(color)
            .padding(.horizontal, 16).padding(.top, 8)
    }

    private func field(_ key: String, _ value: String, _ unit: String? = nil) -> some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(key).font(.caption2).fontWeight(.semibold).foregroundStyle(Theme.text2)
            (Text(value).font(.system(.body, design: .monospaced)).fontWeight(.semibold)
             + Text(unit.map { " \($0)" } ?? "").font(.caption2).foregroundStyle(Theme.text2))
        }
        .frame(maxWidth: .infinity, minHeight: 70, alignment: .leading)
        .padding(10)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius).stroke(Theme.line, lineWidth: 1))
    }

    // MARK: - 取值格式化

    private var tempC: String { snapshot.temperatureMilli.map { String(format: "%.3f", Double($0) / 1000) } ?? "—" }
    private var humPercent: String { snapshot.humidityMilli.map { String(format: "%.3f", Double($0) / 1000) } ?? "—" }
    private var angleDeg: String { snapshot.angleCenti.map { String(format: "%.2f", Double($0) / 100) } ?? "—" }
    private var liveClock: String {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
        return formatter.string(from: Date())
    }
    private var syncText: String {
        session.isTimeSyncing ? (zh ? "同步中" : "Syncing") : (zh ? "同步" : "Sync")
    }

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
        case 0: return zh ? "融合模式" : "Magnetic + Radar"
        case 1: return zh ? "仅地磁" : "Magnetic Only"
        case 2: return zh ? "雷达优先" : "Radar Priority"
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

    private var portText: String {
        guard let port = snapshot.portFunction else { return "—" }
        switch port & 0x03 {
        case 0: return zh ? "双路脉冲" : "Both pulse"
        case 1: return zh ? "口1 开 · 口2 脉冲" : "P1 switch, P2 pulse"
        case 2: return zh ? "口1 脉冲 · 口2 开关" : "P1 pulse, P2 switch"
        default: return zh ? "双路开关" : "Both switch"
        }
    }

    private var smartText: String {
        switch snapshot.smartPower {
        case 1: return zh ? "自动" : "Auto"
        case 0: return zh ? "手动" : "Manual"
        default: return "—"
        }
    }

    private var tzText: String {
        switch snapshot.timezone {
        case 25: return "UTC+03:30"
        case 26: return "UTC+05:30"
        case .some(let v) where v < 13: return "UTC+\(v):00"
        case .some(let v) where v <= 24: return "UTC−\(v - 12):00"
        case .some: return "—"
        case nil: return "—"
        }
    }

    private var alarmText: String {
        switch snapshot.htAlarm {
        case 1: return zh ? "已触发" : "Triggered"
        case 0: return zh ? "正常" : "Normal"
        default: return "—"
        }
    }

    private func alarmStyle(_ value: Int?) -> String {
        switch value {
        case 1: return zh ? "已触发" : "Triggered"
        case 0: return zh ? "正常" : "Normal"
        default: return zh ? "未知" : "Unknown"
        }
    }

    private func of(_ value: Int?) -> String { value.map(String.init) ?? "—" }
    private func of(_ value: Double?) -> String { value.map { String(format: "%.6f", $0) } ?? "—" }

    private func flag(_ value: Int?, yes: String, no: String) -> String {
        switch value {
        case 1: return yes
        case 0: return no
        default: return zh ? "未知" : "Unknown"
        }
    }
}
