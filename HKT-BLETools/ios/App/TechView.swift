import CoreBLE
import CoreProtocol
import SwiftUI

/// P-03b 技术参数页（DC200Family 工程诊断，R-27/SP-27）——1:1 克隆冻结原型。
/// 判定对照（0x3A 车位 / 0x3B 工作模式）+ 地磁三轴最近 30 组折线（X 蓝/Y 绿/Z 橙，随轮询积累）
/// + 雷达 10 段频谱柱状（0x60）。返回-only 导航（R-28：技术参数页不显示首页）。
struct TechView: View {
    let session: DeviceSession
    @Environment(LanguageStore.self) private var langStore
    @Environment(\.dismiss) private var dismiss
    @State private var histX: [Int] = []
    @State private var histY: [Int] = []
    @State private var histZ: [Int] = []
    private let cap = 30

    private var snapshot: DeviceSnapshot { session.snapshot }
    private var zh: Bool { langStore.isZh }

    var body: some View {
        VStack(spacing: 0) {
            NavbarHeader(title: (zh ? "技术参数" : "Tech Parameters") + " · \(session.deviceName)",
                         backText: zh ? "‹ 返回" : "‹ Back",
                         onBack: { dismiss() }) {
                EmptyView()
            }
            ScrollView {
                VStack(alignment: .leading, spacing: 0) {
                    judgeCard
                        .padding(.bottom, 14)
                    SectionHeader(title: zh ? "地磁三轴 · 实时曲线（最近 30 组）" : "Magnetometer · last 30 samples")
                    magCard
                        .padding(.bottom, 14)
                    SectionHeader(title: zh ? "雷达频谱" : "Radar Spectrum")
                    radarCard
                    Text(zh ? "工程诊断页：数据随轮询实时刷新。判读参考——地磁三轴变化量过小说明灵敏度/安装异常；雷达各段能量整体偏低说明有遮挡或干扰。"
                            : "Engineering view: values refresh with each poll. Low magnetometer delta suggests sensitivity/mounting issues; uniformly low radar bins suggest occlusion or interference.")
                        .font(.hkt(11))
                        .foregroundStyle(Theme.text2)
                        .lineSpacing(1.5 * 11 - 11)
                        .padding(.top, 10)
                }
                .padding(.horizontal, 16)
                .padding(.top, 4)
                .padding(.bottom, 24)
            }
        }
        .background(Theme.bg)
        .toolbar(.hidden, for: .navigationBar)
        .onAppear { appendSample() }
        .onChange(of: session.lastResponseAt) { _, _ in appendSample() }
    }

    // MARK: - 样本积累（每轮询应答追加一组，环形 30）

    private func appendSample() {
        guard let x = snapshot.magX, let y = snapshot.magY, let z = snapshot.magZ else { return }
        histX.append(x)
        histY.append(y)
        histZ.append(z)
        if histX.count > cap {
            histX.removeFirst(histX.count - cap)
            histY.removeFirst(histY.count - cap)
            histZ.removeFirst(histZ.count - cap)
        }
    }

    // MARK: - 判定对照（0x3A / 0x3B）

    private var judgeCard: some View {
        let park = snapshot.parkState
        let (parkText, parkKind): (String, BadgeKind)
        switch park {
        case 1: (parkText, parkKind) = (zh ? "有车" : "Occupied", .info)
        case 0: (parkText, parkKind) = (zh ? "空位" : "Vacant", .warn)
        case 255: (parkText, parkKind) = (zh ? "被遮挡" : "Covered", .warn)
        default: (parkText, parkKind) = (zh ? "未知" : "Unknown", .warn)
        }
        let modes = zh ? ["融合模式", "仅地磁", "雷达优先"] : ["Fusion", "Mag only", "Radar first"]
        let mode = snapshot.parkMode.flatMap { modes.indices.contains($0) ? modes[$0] : nil } ?? "-"
        return VStack(alignment: .leading, spacing: 0) {
            Text(zh ? "当前判定" : "Current reading")
                .font(.hkt(12))
                .foregroundStyle(Theme.text2)
            HStack(spacing: 14) {
                StateBadge(kind: parkKind, text: parkText)
                Text((zh ? "工作模式: " : "Work mode: ") + mode)
                    .font(.hkt(13))
                    .foregroundStyle(Theme.text2)
            }
            .padding(.top, 6)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(14)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius).stroke(Theme.line.opacity(0.82), lineWidth: 1))
    }

    // MARK: - 地磁三轴折线（最近 30 组）

    private var magCard: some View {
        VStack(alignment: .leading, spacing: 0) {
            HStack(alignment: .firstTextBaseline) {
                Text(zh ? "地磁 X/Y/Z" : "Mag X/Y/Z")
                    .font(.hkt(14, .bold))
                Spacer()
                if let x = histX.last, let y = histY.last, let z = histZ.last {
                    Text("X \(signed(x))").foregroundStyle(Color(hex: "#007AFF"))
                    Text("Y \(signed(y))").foregroundStyle(Color(hex: "#34C759"))
                    Text("Z \(signed(z))").foregroundStyle(Color(hex: "#FF9500"))
                }
            }
            .font(.hkt(14, .bold))
            .foregroundStyle(Theme.text)
            .padding(.bottom, 8)
            magChart
                .frame(height: 150)
            HStack {
                Text("min \(histMin)")
                Text("\(histX.count) / \(cap)")
                Text("max \(histMax)")
            }
            .font(.hkt(11))
            .monospacedDigit()
            .foregroundStyle(Theme.text2)
            .frame(maxWidth: .infinity)
            .padding(.top, 6)
        }
        .padding(14)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius).stroke(Theme.line.opacity(0.82), lineWidth: 1))
    }

    private var histMin: Int { minOfAll() ?? 0 }
    private var histMax: Int { maxOfAll() ?? 0 }

    private func minOfAll() -> Int? { (histX + histY + histZ).min() }
    private func maxOfAll() -> Int? { (histX + histY + histZ).max() }

    private func signed(_ v: Int) -> String { v > 0 ? "+\(v)" : "\(v)" }

    /// 原型 magChart：4 分格网格 + 过零虚线 + 三色折线 + 末端点，上下留 8% 边距。
    @ViewBuilder
    private var magChart: some View {
        if histX.count < 2 {
            Text(zh ? "正在积累数据（至少 2 组）…" : "Collecting samples (need ≥2)…")
                .font(.hkt(12))
                .foregroundStyle(Theme.text2)
                .frame(maxWidth: .infinity, minHeight: 130)
        } else {
            Canvas { context, size in
                let pad: CGFloat = 8
                let all = histX + histY + histZ
                var lo = CGFloat(all.min() ?? 0)
                var hi = CGFloat(all.max() ?? 1)
                if hi == lo { hi = lo + 1 }
                let span = hi - lo
                lo -= span * 0.08
                hi += span * 0.08
                func y(_ v: Int) -> CGFloat {
                    size.height - pad - (CGFloat(v) - lo) / (hi - lo) * (size.height - 2 * pad)
                }
                func x(_ i: Int, count: Int) -> CGFloat {
                    pad + CGFloat(i) * (size.width - 2 * pad) / CGFloat(max(count - 1, 1))
                }
                // 网格（1/4、2/4、3/4 三条横线）
                for i in 1...3 {
                    let gy = pad + CGFloat(i) * (size.height - 2 * pad) / 4
                    var grid = Path()
                    grid.move(to: CGPoint(x: pad, y: gy))
                    grid.addLine(to: CGPoint(x: size.width - pad, y: gy))
                    context.stroke(grid, with: .color(Theme.line), lineWidth: 1)
                }
                // 过零虚线
                if lo < 0 && hi > 0 {
                    let zy = y(0)
                    var zero = Path()
                    zero.move(to: CGPoint(x: pad, y: zy))
                    zero.addLine(to: CGPoint(x: size.width - pad, y: zy))
                    context.stroke(zero, with: .color(Theme.text2.opacity(0.45)),
                                   style: StrokeStyle(lineWidth: 1, dash: [5, 4]))
                }
                let series: [(Color, [Int])] = [(Color(hex: "#007AFF"), histX),
                                                (Color(hex: "#34C759"), histY),
                                                (Color(hex: "#FF9500"), histZ)]
                for (color, values) in series {
                    guard values.count >= 2 else { continue }
                    var line = Path()
                    for (i, v) in values.enumerated() {
                        let p = CGPoint(x: x(i, count: values.count), y: y(v))
                        i == 0 ? line.move(to: p) : line.addLine(to: p)
                    }
                    context.stroke(line, with: .color(color), style: StrokeStyle(lineWidth: 2, lineCap: .round, lineJoin: .round))
                    if let last = values.last {
                        let dot = CGRect(x: x(values.count - 1, count: values.count) - 3.2,
                                         y: y(last) - 3.2, width: 6.4, height: 6.4)
                        context.fill(Path(ellipseIn: dot), with: .color(color))
                    }
                }
            }
        }
    }

    // MARK: - 雷达 10 段频谱

    @ViewBuilder
    private var radarCard: some View {
        VStack(spacing: 0) {
            if let radar = snapshot.radarSpectrum, !radar.isEmpty {
                let peak = max(radar.max() ?? 0, 1)
                HStack(alignment: .bottom, spacing: 5) {
                    ForEach(radar.indices, id: \.self) { i in
                        VStack(spacing: 4) {
                            RoundedRectangle(cornerRadius: 3)
                                .fill(Theme.info)
                                .frame(height: max(4, CGFloat(radar[i]) / CGFloat(peak) * 130))
                            Text("\(i + 1)")
                                .font(.hkt(9))
                                .foregroundStyle(Theme.text2)
                        }
                        .frame(maxWidth: .infinity)
                    }
                }
                .frame(height: 150)
                Text(radar.map(String.init).joined(separator: " / "))
                    .font(.hkt(11))
                    .monospacedDigit()
                    .foregroundStyle(Theme.text2)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(.top, 6)
            } else {
                Text(zh ? "等待雷达数据…" : "Waiting for radar data…")
                    .font(.hkt(12))
                    .foregroundStyle(Theme.text2)
                    .frame(maxWidth: .infinity, minHeight: 150)
            }
        }
        .padding(14)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius).stroke(Theme.line.opacity(0.82), lineWidth: 1))
    }
}
