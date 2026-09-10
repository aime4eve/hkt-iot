import SwiftUI

// MARK: - 配置页组件（P_config，规格卡 P-config.md；数值逐条来自原型 CSS）

/// .cfg-label：11px/650 text2，margin 10 0 6。
struct ConfigLabel: View {
    let text: String

    var body: some View {
        Text(text)
            .font(.hkt(11, .semibold))
            .foregroundStyle(Theme.text2)
            .padding(.top, 10).padding(.bottom, 6)
    }
}

/// .cfg-caption：11px text2 行高 1.35。
struct ConfigCaption: View {
    let text: String
    var topSpacing: CGFloat = 6

    var body: some View {
        Text(text)
            .font(.hkt(11))
            .lineSpacing(1.35 * 11 - 11)
            .foregroundStyle(Theme.text2)
            .padding(.top, topSpacing)
            .frame(maxWidth: .infinity, alignment: .leading)
    }
}

/// 状态胶囊（.cfg-state：10px/700，card2 底 line 描边；on=ok 色系）。
struct ConfigStatePill: View {
    let text: String
    var on = true

    var body: some View {
        Text(text)
            .font(.hkt(10, .bold))
            .foregroundStyle(on ? Theme.ok : Theme.text2)
            .padding(.horizontal, 7).padding(.vertical, 3)
            .background(on ? Theme.ok.opacity(0.11) : Theme.card2, in: Capsule())
            .overlay(Capsule().stroke(on ? Theme.ok.opacity(0.20) : Theme.line, lineWidth: 1))
    }
}

/// .cfg-section：标题(15/700)+caption+右上状态胶囊+内容。
struct ConfigSection<Content: View>: View {
    let title: String
    var caption: String?
    var state: String?
    @ViewBuilder var content: Content

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            HStack(alignment: .top, spacing: 8) {
                VStack(alignment: .leading, spacing: 2) {
                    Text(title).font(.hkt(15, .bold)).lineSpacing(1.2 * 15 - 15)
                        .foregroundStyle(Theme.text)
                    if let caption {
                        Text(caption).font(.hkt(11)).lineSpacing(1.35 * 11 - 11)
                            .foregroundStyle(Theme.text2)
                    }
                }
                Spacer(minLength: 8)
                if let state { ConfigStatePill(text: state, on: true) }
            }
            .padding(.bottom, 10)
            content
        }
        .padding(12)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
            .stroke(Theme.line.opacity(0.82), lineWidth: 1))
        .hktShadow()
        .padding(.bottom, 10)
    }
}

/// 数字输入行（numRow → .card：label + input+unit + hint + error）。
struct NumFieldCard: View {
    let label: String
    @Binding var text: String
    var unit: String?
    var hint: String?
    var error: String?
    var disabled = false

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            Text(label).font(.hkt(12)).foregroundStyle(Theme.text2)
            HStack(spacing: 8) {
                TextField("", text: $text)
                    .keyboardType(.numberPad)
                    .font(.hkt(14))
                    .foregroundStyle(Theme.text)
                    .disabled(disabled)
                    .opacity(disabled ? 0.5 : 1)
                if let unit, !unit.isEmpty {
                    Text(unit).font(.hkt(12)).foregroundStyle(Theme.text2)
                }
            }
            .padding(.top, 6)
            if let hint {
                Text(hint).font(.hkt(11)).foregroundStyle(Theme.text2)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(.top, 4)
            }
            if let error {
                Text("✕ " + error).font(.hkt(12)).foregroundStyle(Theme.err)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(.top, 4)
            }
        }
        .padding(EdgeInsets(top: 13, leading: 14, bottom: 13, trailing: 14))
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
            .stroke(Theme.line.opacity(0.82), lineWidth: 1))
        .hktShadow()
    }
}

/// 单选芯片组（segRow：card 内 label + 等宽芯片；选中=info 底白字 600，未选=card2 底 text2 400）。
struct ChoiceChipRow: View {
    let label: String
    let options: [String]
    @Binding var selection: Int

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            Text(label).font(.hkt(12)).foregroundStyle(Theme.text2)
            HStack(spacing: 6) {
                ForEach(options.indices, id: \.self) { index in
                    let selected = selection == index
                    Button {
                        selection = index
                    } label: {
                        Text(options[index])
                            .font(.hkt(13, selected ? .semibold : .regular))
                            .foregroundStyle(selected ? .white : Theme.text2)
                            .frame(maxWidth: .infinity)
                            .padding(.vertical, 9).padding(.horizontal, 4)
                            .background(selected ? Theme.info : Theme.card2,
                                        in: RoundedRectangle(cornerRadius: 9))
                    }
                }
            }
            .padding(.top, 8)
        }
        .padding(EdgeInsets(top: 13, leading: 14, bottom: 13, trailing: 14))
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
            .stroke(Theme.line.opacity(0.82), lineWidth: 1))
        .hktShadow()
    }
}

/// .choice（cfg-grid 3 列：选中=info 10% 底 info 字 info 描边）。
struct ChoiceCell: View {
    let text: String
    let selected: Bool
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            Text(text)
                .font(.hkt(15, .bold))
                .monospacedDigit()
                .foregroundStyle(selected ? Theme.info : Theme.text)
                .frame(maxWidth: .infinity)
                .padding(.vertical, 10).padding(.horizontal, 6)
                .background(selected ? Theme.info.opacity(0.10) : Theme.card2,
                            in: RoundedRectangle(cornerRadius: Theme.controlRadius))
                .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius)
                    .stroke(selected ? Theme.info.opacity(0.35) : Theme.line, lineWidth: 1))
        }
    }
}

/// .mode-option（端口组内 开关控制/PWM 控制：选中=info 10%底 info 字）。
struct ModeOption: View {
    let text: String
    let selected: Bool
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            Text(text)
                .font(.hkt(12, .semibold))
                .lineSpacing(1.25 * 12 - 12)
                .multilineTextAlignment(.center)
                .foregroundStyle(selected ? Theme.info : Theme.text2)
                .frame(maxWidth: .infinity)
                .padding(.vertical, 8).padding(.horizontal, 6)
                .background(selected ? Theme.info.opacity(0.10) : Theme.card,
                            in: RoundedRectangle(cornerRadius: 7))
                .overlay(RoundedRectangle(cornerRadius: 7)
                    .stroke(selected ? Theme.info.opacity(0.32) : Theme.line, lineWidth: 1))
        }
    }
}

/// .port-set（阀 1/阀 2 端口组：card2 底 r8，标题 11px/700 text2 + 两组 mode-option）。
struct PortSet: View {
    let title: String
    let onText: String        // 开关控制
    let offText: String       // PWM 控制
    let isOn: Bool            // 该路当前是否开关模式
    let setOn: (Bool) -> Void

    var body: some View {
        VStack(alignment: .leading, spacing: 5) {
            Text(title).font(.hkt(11, .bold)).foregroundStyle(Theme.text2)
                .padding(.bottom, 2)
            ModeOption(text: onText, selected: isOn) { setOn(true) }
            ModeOption(text: offText, selected: !isOn) { setOn(false) }
        }
        .padding(10)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Theme.card2, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius)
            .stroke(Theme.line, lineWidth: 1))
    }
}

/// .cfg-input-row（输入 + 单位；disabled=opacity .5）。
struct ConfigInputRow: View {
    let label: String
    @Binding var text: String
    var unit: String?
    var disabled = false

    var body: some View {
        HStack {
            TextField("", text: $text)
                .keyboardType(.numberPad)
                .font(.hkt(15, .semibold))
                .monospacedDigit()
                .foregroundStyle(Theme.text)
                .disabled(disabled)
            if let unit {
                Text(unit).font(.hkt(12)).foregroundStyle(Theme.text2)
            }
        }
        .padding(.horizontal, 10).padding(.vertical, 10)
        .background(Theme.card2, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius)
            .stroke(Theme.line, lineWidth: 1))
        .opacity(disabled ? 0.5 : 1)
    }
}

/// .cfg-control（label 左 + 自定义右侧，如 switch；card2 底 r8）。
struct ConfigControlRow<Trailing: View>: View {
    let label: String
    @ViewBuilder var trailing: Trailing

    var body: some View {
        HStack {
            Text(label).font(.hkt(13, .semibold)).foregroundStyle(Theme.text)
            Spacer(minLength: 8)
            trailing
        }
        .padding(.horizontal, 10).padding(.vertical, 9)
        .background(Theme.card2, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius)
            .stroke(Theme.line, lineWidth: 1))
    }
}
