import SwiftUI

// swiftlint:disable file_length
// MARK: - 字号/字重基元（对应原型 CSS px 与数值字重）

/// 原型字号全部为固定 px，禁用 SwiftUI 语义字体（正文/标题等）近似。
/// CSS 数值字重映射：400→.regular，550/600/650→.semibold，700/750→.bold。
extension Font {
    static func hkt(_ size: CGFloat, _ weight: Font.Weight = .regular) -> Font {
        .system(size: size, weight: weight)
    }
}

// MARK: - 卡片物理（阴影，数值=规格卡 §2）

/// 卡片阴影：light `0 1 2 rgba(15,23,42,.05)` / dark `0 1 2 rgba(0,0,0,.24)`。
extension View {
    func hktShadow() -> some View {
        modifier(HKTShadowModifier())
    }
}

struct HKTShadowModifier: ViewModifier {
    @Environment(\.colorScheme) private var scheme
    func body(content: Content) -> some View {
        content.shadow(
            color: scheme == .dark
                ? Color.black.opacity(0.24)
                : Color(red: 15 / 255, green: 23 / 255, blue: 42 / 255).opacity(0.05),
            radius: 1, x: 0, y: 1)
    }
}

/// 卡片容器（.card）：card 底、`line 82%` 描边、r10、阴影；padding 由调用方给（.card=13/14，.field=10/12，.svc-module=12）。
struct HKTCard<Content: View>: View {
    var radius: CGFloat = Theme.cardRadius
    var padding: EdgeInsets
    var borderOpacity: Double = 0.82
    @ViewBuilder var content: Content

    init(radius: CGFloat = Theme.cardRadius,
         padding: EdgeInsets = EdgeInsets(top: 13, leading: 14, bottom: 13, trailing: 14),
         borderOpacity: Double = 0.82,
         @ViewBuilder content: () -> Content) {
        self.radius = radius
        self.padding = padding
        self.borderOpacity = borderOpacity
        self.content = content()
    }

    var body: some View {
        content
            .padding(padding)
            .background(Theme.card, in: RoundedRectangle(cornerRadius: radius))
            .overlay(RoundedRectangle(cornerRadius: radius).stroke(Theme.line.opacity(borderOpacity), lineWidth: 1))
            .hktShadow()
    }
}

// MARK: - SectionHeader（.section）

struct SectionHeader: View {
    let title: String
    var body: some View {
        HStack(spacing: 8) {
            Text(title)
                .font(.hkt(11, .bold))
                .tracking(0.55)             // letter-spacing .05em @ 11px
                .foregroundStyle(Theme.text2)
            Rectangle().fill(Theme.line.opacity(0.76)).frame(height: 1)
        }
        .padding(.top, 16).padding(.horizontal, 4).padding(.bottom, 8)
    }
}

// MARK: - FieldTile（.field）与 FieldGrid（.fieldgrid）

struct FieldSpec {
    let label: String
    let value: String
    var unit: String?
    init(_ label: String, _ value: String, unit: String? = nil) {
        self.label = label
        self.value = value
        self.unit = unit
    }
}

/// 数据驱动的两列字段网格；奇数个字段时末项独占整行（.fieldgrid odd-last 规则）。
struct FieldGrid: View {
    let fields: [FieldSpec]
    private var rows: [[FieldSpec]] {
        stride(from: 0, to: fields.count, by: 2).map {
            Array(fields[$0 ..< min($0 + 2, fields.count)])
        }
    }

    var body: some View {
        VStack(spacing: 8) {
            ForEach(rows.indices, id: \.self) { r in
                HStack(spacing: 8) {
                    ForEach(rows[r].indices, id: \.self) { c in
                        FieldTile(spec: rows[r][c])
                    }
                    if rows[r].count == 1 { Spacer() }
                }
            }
        }
    }
}

struct FieldTile: View {
    let spec: FieldSpec

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            Text(spec.label)
                .font(.hkt(11, .semibold))
                .foregroundStyle(Theme.text2)
                .lineLimit(1)
                .truncationMode(.tail)
            Spacer(minLength: 4)                        // .field 纵向 space-between
            valueText
                .font(.hkt(16, .semibold))
                .monospacedDigit()
                .foregroundStyle(Theme.text)
                .lineSpacing(1.25 * 16 - 16)
                .fixedSize(horizontal: false, vertical: true)
        }
        .frame(maxWidth: .infinity, alignment: .topLeading)
        .padding(EdgeInsets(top: 10, leading: 12, bottom: 10, trailing: 12))
        // CSS min-height:70 是 border-box 总高 → frame 必须在 padding 之后
        .frame(minHeight: 70, alignment: .topLeading)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
            .stroke(Theme.line.opacity(0.82), lineWidth: 1))
        .hktShadow()
    }

    /// 值 + 小号单位 span（.v .unit：11px text2/400，左距 2）。
    private var valueText: Text {
        let v = Text(spec.value)
        guard let unit = spec.unit else { return v }
        return v + Text(" \(unit)").font(.hkt(11)).foregroundColor(Theme.text2)
    }
}

// MARK: - 状态徽章（.badge + .dot）

enum BadgeKind { case ok, warn, err, info }

struct StateBadge: View {
    let kind: BadgeKind
    let text: String
    var compact = false        // liveCard 变体：padding 2 8（标准 3 9）

    private var color: Color {
        switch kind {
        case .ok: return Theme.ok
        case .warn: return Theme.warn
        case .err: return Theme.err
        case .info: return Theme.info
        }
    }

    var body: some View {
        HStack(spacing: 5) {
            Circle().fill(color).frame(width: 7, height: 7)     // .dot 7×7 currentColor
            Text(text).font(.hkt(12, .semibold))
        }
        .padding(.horizontal, compact ? 8 : 9)
        .padding(.vertical, compact ? 2 : 3)
        .background(color.opacity(0.15), in: Capsule())
        .foregroundStyle(color)
    }
}

// MARK: - 通道状态胶囊（.state-pill on/off）

struct StatePill: View {
    let on: Bool
    let text: String

    var body: some View {
        HStack(spacing: 4) {
            Circle().fill(on ? Theme.ok : Theme.text2).frame(width: 7, height: 7)
            Text(text).font(.hkt(11, .bold))
        }
        .padding(.horizontal, 7).padding(.vertical, 3)
        .background(on ? Theme.ok.opacity(0.14) : Theme.text2.opacity(0.10), in: Capsule())
        .foregroundStyle(on ? Theme.ok : Theme.text2)
    }
}

// MARK: - 会话控制卡（.sessionbar，SP-G6）

struct SessionControlCard<Actions: View>: View {
    let deviceName: String
    let meta: String
    var badge: StateBadge?
    @ViewBuilder var actions: Actions

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            HStack(alignment: .center) {
                Text(deviceName)
                    .font(.hkt(23, .bold))          // 23px / 750
                    .lineSpacing(23 * 1.1 - 23)
                Spacer(minLength: 10)
                if let badge { badge }
            }
            Text(meta)
                .font(.hkt(12))
                .monospacedDigit()
                .foregroundStyle(Theme.text2)
                .frame(maxWidth: .infinity, alignment: .trailing)   // .device-meta 右对齐
                .padding(.top, 5).padding(.bottom, 12)
            LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())], spacing: 6) {
                actions                                             // .session-actions 2×2
            }
        }
        .padding(EdgeInsets(top: 13, leading: 14, bottom: 13, trailing: 14))
        .frame(maxWidth: .infinity)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: 12))     // sessionbar r12
        .overlay(RoundedRectangle(cornerRadius: 12).stroke(Theme.line.opacity(0.82), lineWidth: 1))
        .hktShadow()
        .padding(.horizontal, 16).padding(.bottom, 12)
    }
}

/// .session-btn：高 34、r8、card2 底、`line` 描边、12px/650；danger=err 8%/24%/err。
struct SessionButton: View {
    let title: String
    var danger = false
    var disabled = false
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            Text(title)
                .font(.hkt(12, .semibold))
                .lineLimit(1)
                .minimumScaleFactor(0.7)
                .foregroundStyle(danger ? Theme.err : Theme.text)
                .frame(maxWidth: .infinity).frame(height: 34)
                .background(danger ? Theme.err.opacity(0.08) : Theme.card2,
                            in: RoundedRectangle(cornerRadius: Theme.controlRadius))
                .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius)
                    .stroke(danger ? Theme.err.opacity(0.24) : Theme.line, lineWidth: 1))
        }
        .disabled(disabled)
        .opacity(disabled ? 0.4 : 1)
    }
}

// MARK: - 横幅（.banner err/warn/info）

struct HKTBanner: View {
    let kind: BadgeKind
    let text: String
    var actionTitle: String?
    var action: (() -> Void)?

    private var color: Color {
        switch kind {
        case .ok: return Theme.ok
        case .warn: return Theme.warn
        case .err: return Theme.err
        case .info: return Theme.info
        }
    }

    var body: some View {
        HStack(alignment: .top, spacing: 8) {
            Text(text).font(.hkt(13)).lineSpacing(1.4 * 13 - 13)
            Spacer(minLength: 8)
            if let actionTitle, let action {
                Button(actionTitle, action: action)
                    .font(.hkt(13, .semibold))
                    .foregroundStyle(color)
                    .lineLimit(1)
            }
        }
        .foregroundStyle(color)
        .padding(EdgeInsets(top: 10, leading: 12, bottom: 10, trailing: 12))
        .background(color.opacity(0.12), in: RoundedRectangle(cornerRadius: 10))
        .padding(.bottom, 11)
    }
}

// MARK: - 操作面板（.ops-panel：电源整行 + ops-grid）

struct OpsPanel<Grid: View>: View {
    var powerRow: OpsPowerRow
    @ViewBuilder var grid: Grid

    var body: some View {
        VStack(spacing: 0) {
            powerRow
            VStack(spacing: 8) { grid }         // 本页 op-card 全部 wide=独占整行
                .padding(10)
                .background(Theme.card2)
        }
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius).stroke(Theme.line.opacity(0.82), lineWidth: 1))
        .hktShadow()
    }
}

// MARK: - 页头导航条（.navbar.small：back 胶囊 + 标题 + 右侧 linkbtn）

struct NavbarHeader<Trailing: View>: View {
    let title: String
    var backText: String = "‹ 返回"
    var onBack: () -> Void
    @ViewBuilder var trailing: Trailing

    init(title: String, backText: String = "‹ 返回", onBack: @escaping () -> Void,
         @ViewBuilder trailing: () -> Trailing) {
        self.title = title
        self.backText = backText
        self.onBack = onBack
        self.trailing = trailing()
    }

    var body: some View {
        HStack(spacing: 8) {
            Button(action: onBack) {
                Text(backText)
                    .font(.hkt(14, .semibold))
                    .foregroundStyle(Theme.info)
                    .padding(.horizontal, 9).padding(.vertical, 5)
                    .background(Theme.info.opacity(0.09), in: RoundedRectangle(cornerRadius: Theme.controlRadius))
            }
            Text(title).font(.hkt(17, .bold)).foregroundStyle(Theme.text).lineLimit(1)
            Spacer(minLength: 8)
            trailing
        }
        .padding(.top, 6).padding(.horizontal, 16).padding(.bottom, 6)
    }
}

/// .linkbtn：info 字色 + info 9% 底 + r8。
struct LinkButton: View {
    let title: String
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            Text(title)
                .font(.hkt(14, .semibold))
                .foregroundStyle(Theme.info)
                .padding(.horizontal, 9).padding(.vertical, 6)
                .background(Theme.info.opacity(0.09), in: RoundedRectangle(cornerRadius: Theme.controlRadius))
        }
    }
}

/// 电源整行（.ops-power：min-height 58、底边 line 72%、开关 44×26）。
struct OpsPowerRow: View {
    let label: String
    let stateText: String
    let isOn: Bool
    var disabled = false
    let onToggle: () -> Void

    var body: some View {
        Button(action: onToggle) {
            HStack(spacing: 12) {
                Text(label).font(.hkt(15, .semibold)).foregroundStyle(Theme.text)
                Text(stateText).font(.hkt(12)).foregroundStyle(Theme.text2)
                Spacer()
                HKTToggle(isOn: isOn)
            }
            .padding(.horizontal, 14)
            .frame(minHeight: 58)
            .contentShape(Rectangle())
        }
        .disabled(disabled)
        .opacity(disabled ? 0.45 : 1)
    }
}

/// .switch：44×26 r13，off=fill 底、on=ok 底，旋钮 22×22 白、边距 2、on 左移 20。
struct HKTToggle: View {
    let isOn: Bool

    var body: some View {
        ZStack(alignment: .leading) {
            Capsule().fill(isOn ? Theme.ok : Theme.fill).frame(width: 44, height: 26)
            Circle().fill(.white)
                .frame(width: 22, height: 22)
                .shadow(color: .black.opacity(0.3), radius: 1.5, x: 0, y: 1)
                .offset(x: isOn ? 20 : 2)
        }
        .animation(.easeInOut(duration: 0.2), value: isOn)
    }
}

// MARK: - 操作卡（.op-card wide，徽标 38×38）

enum OpBadgeKind { case info, warn, ok }

enum OpTrailing {
    case go                                  // "›" 17px text2
    case pill(String)                        // .op-action 胶囊
}

struct OpCard: View {
    let badge: String
    var badgeKind: OpBadgeKind = .info
    let title: String
    let desc: String
    var trailing: OpTrailing = .go
    var disabled = false
    let action: () -> Void

    init(badge: String, badgeKind: OpBadgeKind = .info, title: String, desc: String,
         trailing: OpTrailing = .go, disabled: Bool = false, action: @escaping () -> Void) {
        self.badge = badge
        self.badgeKind = badgeKind
        self.title = title
        self.desc = desc
        self.trailing = trailing
        self.disabled = disabled
        self.action = action
    }

    private var badgeColor: Color {
        switch badgeKind {
        case .info: return Theme.info
        case .warn: return Theme.warn
        case .ok: return Theme.ok
        }
    }

    var body: some View {
        Button(action: action) {
            HStack(spacing: 10) {
                Text(badge)
                    .font(.system(size: 10, weight: .bold, design: .monospaced))
                    .tracking(0.4)
                    .foregroundStyle(badgeColor)
                    .frame(width: 38, height: 38)
                    .background(badgeOpacity, in: RoundedRectangle(cornerRadius: 10))
                VStack(alignment: .leading, spacing: 3) {
                    Text(title).font(.hkt(13, .semibold)).lineSpacing(1.2 * 13 - 13)
                        .foregroundStyle(Theme.text)
                        .fixedSize(horizontal: false, vertical: true)
                    Text(desc).font(.hkt(11)).lineSpacing(1.25 * 11 - 11)
                        .foregroundStyle(Theme.text2)
                        .fixedSize(horizontal: false, vertical: true)
                }
                Spacer(minLength: 0)
                trailingView
            }
            .padding(11)
            .frame(maxWidth: .infinity, minHeight: 62, alignment: .leading)
            .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
            .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius)
                .stroke(Theme.line.opacity(0.82), lineWidth: 1))
            .contentShape(Rectangle())
        }
        .disabled(disabled)
        .opacity(disabled ? 0.45 : 1)
        .frame(maxWidth: .infinity)
    }

    @ViewBuilder private var trailingView: some View {
        switch trailing {
        case .go:
            Text("›").font(.hkt(17)).foregroundStyle(Theme.text2)
        case .pill(let text):
            Text(text)
                .font(.hkt(11, .bold))
                .foregroundStyle(Theme.info)
                .padding(.horizontal, 8).padding(.vertical, 6)
                .background(Theme.info.opacity(0.10), in: Capsule())
                .lineLimit(1)
        }
    }

    /// 徽标底色：info 10% / warn 12% / ok 12%（.op-badge 及变体）。
    private var badgeOpacity: Color {
        switch badgeKind {
        case .info: return Theme.info.opacity(0.10)
        case .warn: return Theme.warn.opacity(0.12)
        case .ok: return Theme.ok.opacity(0.12)
        }
    }
}

// MARK: - SVC 阀通道模块（.svc-module / .channel，规格卡 §3.4）

struct SvcChannelSpec {
    let name: String            // 阀 1 / 阀 2
    let on: Bool
    let onText: String          // 开启 / 关闭
    let insertLabel: String     // 插入检测
    let insertText: String      // 是 / 否
    let pulseLabel: String      // 脉冲计数
    let pulseText: String       // 千分位脉冲数
    let portLabel: String       // 端口功能
    let portText: String        // 开关控制 / PWM 控制
}

/// SVC100 阀状态模块：标题+模式 tag，内部 2 列通道卡（card2 底、line 72%、r8、padding 10）。
struct SvcChannelModule: View {
    let title: String
    let tag: String
    let channels: [SvcChannelSpec]

    var body: some View {
        VStack(spacing: 10) {
            HStack {
                Text(title).font(.hkt(13, .bold)).foregroundStyle(Theme.text)
                Spacer(minLength: 8)
                Text(tag)
                    .font(.hkt(10, .semibold))
                    .foregroundStyle(Theme.text2)
                    .padding(.horizontal, 7).padding(.vertical, 3)
                    .background(Theme.card2, in: Capsule())
                    .overlay(Capsule().stroke(Theme.line, lineWidth: 1))
                    .lineLimit(1)
            }
            LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())], spacing: 8) {
                ForEach(channels.indices, id: \.self) { i in
                    channel(channels[i])
                }
            }
        }
        .padding(12)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
            .stroke(Theme.line.opacity(0.82), lineWidth: 1))
        .hktShadow()
    }

    private func channel(_ c: SvcChannelSpec) -> some View {
        VStack(alignment: .leading, spacing: 0) {
            HStack {
                Text(c.name).font(.hkt(12, .bold)).foregroundStyle(Theme.text2)
                Spacer(minLength: 6)
                StatePill(on: c.on, text: c.onText)
            }
            VStack(alignment: .leading, spacing: 8) {           // .channel-kpis
                kpi(c.insertLabel, c.insertText)
                kpi(c.pulseLabel, c.pulseText)
                kpi(c.portLabel, c.portText)
            }
            .padding(.top, 9)
        }
        .padding(10)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Theme.card2, in: RoundedRectangle(cornerRadius: 8))
        .overlay(RoundedRectangle(cornerRadius: 8).stroke(Theme.line.opacity(0.72), lineWidth: 1))
    }

    @ViewBuilder private func kpi(_ label: String, _ value: String) -> some View {
        VStack(alignment: .leading, spacing: 1) {
            Text(label).font(.hkt(10, .semibold)).foregroundStyle(Theme.text2).lineLimit(1)
            Text(value).font(.hkt(14, .semibold)).monospacedDigit()
                .foregroundStyle(Theme.text).lineSpacing(1.2 * 14 - 14)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }
}

struct HKTDialog<Buttons: View>: View {
    let title: String
    var centeredBody = false
    let message: Text
    @ViewBuilder var buttons: Buttons

    init(title: String, centeredBody: Bool = false, message: Text,
         @ViewBuilder buttons: () -> Buttons) {
        self.title = title
        self.centeredBody = centeredBody
        self.message = message
        self.buttons = buttons()
    }

    init(title: String, centeredBody: Bool = false, message: String,
         @ViewBuilder buttons: () -> Buttons) {
        self.init(title: title, centeredBody: centeredBody, message: Text(message), buttons: buttons)
    }

    var body: some View {
        ZStack {
            Color.black.opacity(0.42).ignoresSafeArea()
            VStack(spacing: 0) {
                Text(title).font(.hkt(16, .bold)).multilineTextAlignment(.center)
                    .padding(.bottom, 8)
                message
                    .font(.hkt(13))
                    .lineSpacing(1.5 * 13 - 13)
                    .foregroundStyle(Theme.text2)
                    .multilineTextAlignment(centeredBody ? .center : .leading)
                    .frame(maxWidth: .infinity, alignment: centeredBody ? .center : .leading)
                    .padding(.bottom, 14)
                HStack(spacing: 10) { buttons }
            }
            .padding(EdgeInsets(top: 20, leading: 18, bottom: 20, trailing: 18))
            .frame(width: 296)
            .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.dialogRadius))
        }
    }
}

/// .btn：全宽 padding 13、r8、16px/650。primary=info底白字；secondary=card底 line描边；danger=err 10%/22%/err 字。
struct DialogButton: View {
    enum Kind { case primary, secondary, danger }
    let title: String
    var kind: Kind = .secondary
    let action: () -> Void

    private var foreground: Color {
        switch kind {
        case .primary: return .white
        case .secondary: return Theme.text
        case .danger: return Theme.err
        }
    }

    private var background: Color {
        switch kind {
        case .primary: return Theme.info
        case .secondary: return Theme.card
        case .danger: return Theme.err.opacity(0.10)
        }
    }

    private var border: Color? {
        switch kind {
        case .primary: return nil
        case .secondary: return Theme.line
        case .danger: return Theme.err.opacity(0.22)
        }
    }

    var body: some View {
        Button(action: action) {
            Text(title)
                .font(.hkt(16, .semibold))
                .foregroundStyle(foreground)
                .frame(maxWidth: .infinity)
                .padding(13)
                .background(background, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
                .overlay {
                    if let border {
                        RoundedRectangle(cornerRadius: Theme.controlRadius).stroke(border, lineWidth: 1)
                    }
                }
        }
    }
}

// MARK: - 分段进度条（.steps：P-02 连接三阶段）

struct Steps: View {
    let total: Int
    /// 已点亮的段数（原型 `i<=connStep`：阶段 0 → 1 亮）。
    let onCount: Int

    var body: some View {
        HStack(spacing: 6) {
            ForEach(0..<total, id: \.self) { index in
                RoundedRectangle(cornerRadius: 3)
                    .fill(index < onCount ? Theme.info : Theme.fill)
                    .frame(width: 24, height: 5)
            }
        }
        .padding(.vertical, 8)          // .steps margin 8 0
    }
}

// MARK: - 大标题页头（.navbar 大标题版：P-01 扫描页）

struct NavbarLarge<Trailing: View>: View {
    let title: String
    @ViewBuilder var trailing: Trailing

    var body: some View {
        HStack(spacing: 8) {
            Text(title).font(.hkt(20, .bold)).foregroundStyle(Theme.text)
            Spacer(minLength: 8)
            trailing
        }
        .padding(.top, 10).padding(.horizontal, 16).padding(.bottom, 10)
    }
}

// MARK: - 扫描状态行（.row：P-01）

struct ScanStatusRow: View {
    let title: String
    let actionTitle: String     // 停止 / 重新扫描
    let action: () -> Void

    var body: some View {
        HStack {
            Text(title).font(.hkt(15)).foregroundStyle(Theme.text)
            Spacer(minLength: 8)
            LinkButton(title: actionTitle, action: action)
        }
        .padding(EdgeInsets(top: 12, leading: 14, bottom: 12, trailing: 14))
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
            .stroke(Theme.line.opacity(0.82), lineWidth: 1))
        .hktShadow()
        .padding(.bottom, 9)
    }
}

// MARK: - RSSI 信号条（.rssi：4 根竖条，亮=ok、灭=fill）

struct RssiBars: View {
    /// 亮起的条数（映射：rssi>-70→4，>-85→3，否则 2；原型 .rssi s1/s2/s3）
    let lit: Int

    private let heights: [CGFloat] = [4, 7, 10, 14]

    var body: some View {
        HStack(alignment: .bottom, spacing: 2) {
            ForEach(0 ..< 4, id: \.self) { i in
                RoundedRectangle(cornerRadius: 1)
                    .fill(i < lit ? Theme.ok : Theme.fill)
                    .frame(width: 3, height: heights[i])
            }
        }
        .frame(height: 14, alignment: .bottom)
    }
}

// MARK: - 扫描设备卡（.card hcard：P-01；也用于驻留卡/最近设备卡）

struct ScanDeviceCard<Trailing: View>: View {
    let name: String
    let subtitle: String
    var badge: StateBadge?
    @ViewBuilder var trailing: Trailing

    var body: some View {
        HStack(spacing: 12) {
            VStack(alignment: .leading, spacing: 2) {
                HStack(spacing: 6) {
                    Text(name).font(.hkt(15, .bold)).foregroundStyle(Theme.text).lineLimit(1)
                    if let badge { badge }
                }
                Text(subtitle)
                    .font(.hkt(12))
                    .monospacedDigit()
                    .foregroundStyle(Theme.text2)
                    .lineLimit(1)
            }
            Spacer(minLength: 8)
            trailing
        }
        .padding(EdgeInsets(top: 13, leading: 14, bottom: 13, trailing: 14))
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
            .stroke(Theme.line.opacity(0.82), lineWidth: 1))
        .hktShadow()
    }
}

// MARK: - 设置行（.row：P-07；label 左 + 自定义 value 右）

struct SettingsRow<Value: View>: View {
    let label: String
    @ViewBuilder var value: Value
    var action: (() -> Void)? = nil

    var body: some View {
        HStack {
            Text(label).font(.hkt(15)).foregroundStyle(Theme.text)
            Spacer(minLength: 8)
            value
        }
        .padding(EdgeInsets(top: 12, leading: 14, bottom: 12, trailing: 14))
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
            .stroke(Theme.line.opacity(0.82), lineWidth: 1))
        .hktShadow()
        .padding(.bottom, 9)
        .modifier(SettingsRowTap(action: action))
    }
}

private struct SettingsRowTap: ViewModifier {
    let action: (() -> Void)?
    func body(content: Content) -> some View {
        if let action {
            content.contentShape(Rectangle()).onTapGesture(perform: action)
        } else {
            content
        }
    }
}

/// 行右值样式（.row .value：14px text2，横排 gap 6）。
struct RowValue<Content: View>: View {
    @ViewBuilder var content: Content

    var body: some View {
        HStack(spacing: 6) {
            content
        }
        .font(.hkt(14))
        .foregroundStyle(Theme.text2)
    }
}

// MARK: - 前缀过滤芯片（P-07：选中 info 底白字 / 未选中 card2 底 text2）

struct PrefixChip: View {
    let text: String
    let selected: Bool
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            Text(text)
                .font(.hkt(13, .semibold))
                .foregroundStyle(selected ? .white : Theme.text2)
                .frame(maxWidth: .infinity)
                .padding(.vertical, 8)
                .background(selected ? Theme.info : Theme.card2,
                            in: RoundedRectangle(cornerRadius: 9))
        }
    }
}

// MARK: - 日志行（.logline：等宽 11px、底边 line、级别色）

struct LogLine: View {
    let timestamp: String
    let level: String          // INFO / WARN / ERR
    let message: String

    private var levelColor: Color {
        switch level {
        case "ERR": return Theme.err
        case "WARN": return Theme.warn
        default: return Theme.info
        }
    }

    var body: some View {
        HStack(alignment: .top, spacing: 0) {
            Text(timestamp)
                .font(.system(size: 11, weight: .regular, design: .monospaced))
                .foregroundStyle(Theme.text2)
                .padding(.trailing, 6)
            Text(level)
                .font(.system(size: 11, weight: .bold, design: .monospaced))
                .foregroundStyle(levelColor)
                .frame(width: 44, alignment: .leading)
            Text(message)
                .font(.system(size: 11, weight: .regular, design: .monospaced))
                .foregroundStyle(Theme.text)
        }
        .lineSpacing(1.8 * 11 - 11)
        .padding(.vertical, 2)
        .frame(maxWidth: .infinity, alignment: .leading)
        .overlay(alignment: .bottom) {
            Rectangle().fill(Theme.line).frame(height: 1)
        }
    }
}

// MARK: - 校准步骤行（P_cal：圆形序号 + 可选粗体标题 + 文本）

struct CalStepRow: View {
    let index: Int            // 1 起；0 = 无序号（UDS 纯文本步骤）
    let title: String?        // 可选粗体标题（DC 7 项有，UDS 无）
    let text: String

    var body: some View {
        HStack(alignment: .top, spacing: 12) {
            if index > 0 {
                Text("\(index)")
                    .font(.hkt(13, .bold))
                    .foregroundStyle(.white)
                    .frame(width: 26, height: 26)
                    .background(Theme.info, in: Circle())
            }
            Group {
                if let title, !title.isEmpty {
                    Text(title + ": ").bold() + Text(text)
                } else {
                    Text(text)
                }
            }
            .font(.hkt(13))
            .lineSpacing(1.6 * 13 - 13)
            .foregroundStyle(Theme.text2)
            .padding(.top, index > 0 ? 3 : 0)
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .padding(.vertical, 12)
    }
}

// MARK: - 进度条（.progress：高 8 圆角 4，fill 底 info 填充）

struct HKTProgress: View {
    /// 0...1
    let fraction: Double

    var body: some View {
        GeometryReader { proxy in
            ZStack(alignment: .leading) {
                RoundedRectangle(cornerRadius: 4).fill(Theme.fill)
                RoundedRectangle(cornerRadius: 4).fill(Theme.info)
                    .frame(width: max(0, min(1, fraction)) * proxy.size.width)
            }
        }
        .frame(height: 8)
    }
}

// MARK: - 居中早退视图（.center：⚠︎/📵/⏻ 大图标 + 标题 + 按钮）

struct CenterStateView: View {
    let glyph: String
    let glyphSize: CGFloat
    let title: String
    var subtitle: String?
    let buttonTitle: String
    var secondaryButton = false    // .btn secondary：card 底 line 描边 text 字（如空态「重新扫描」）
    var fillsRemaining = true      // true=撑满剩余空间（整页早退视图）；false=内容高（滚动区空态）
    let action: () -> Void

    var body: some View {
        core
            .frame(maxWidth: .infinity)
            .modifier(CenterFills(fillsRemaining: fillsRemaining))
    }

    private var core: some View {
        VStack(spacing: 10) {
            Text(glyph).font(.system(size: glyphSize))
            Text(title).font(.hkt(17, .semibold))
            if let subtitle {
                Text(subtitle).font(.hkt(13)).lineSpacing(1.5 * 13 - 13)
                    .foregroundStyle(Theme.text2)
                    .multilineTextAlignment(.center)
            }
            if !buttonTitle.isEmpty {
                Button(action: action) {
                    Text(buttonTitle)
                        .font(.hkt(16, .semibold))
                        .foregroundStyle(secondaryButton ? Theme.text : .white)
                        .padding(.vertical, secondaryButton ? 10 : 12)
                        .padding(.horizontal, secondaryButton ? 26 : 34)
                        .background(secondaryButton ? Theme.card : Theme.info,
                                    in: RoundedRectangle(cornerRadius: Theme.controlRadius))
                        .overlay {
                            if secondaryButton {
                                RoundedRectangle(cornerRadius: Theme.controlRadius)
                                    .stroke(Theme.line, lineWidth: 1)
                            }
                        }
                }
                .padding(.top, 2)
            }
        }
        .padding(.horizontal, 32)
    }
}

private struct CenterFills: ViewModifier {
    let fillsRemaining: Bool
    func body(content: Content) -> some View {
        if fillsRemaining {
            content.frame(maxHeight: .infinity)
        } else {
            content.frame(minHeight: 320)   // 原型 .center min-height:320（P-01 空态）
        }
    }
}
