import CoreBLE
import SwiftUI

/// R-2 目标设备定位 UI —— 1:1 克隆冻结原型（定位 sheet / 输入对话框 / 相机取景 / 定位中脉冲 / 未找到）。
/// 状态机 = ScanModel.locatePhase（input/finding/notFound，30s 超时未找到，命中 onLocateHit→连接覆盖层）。
/// 相机取景为模拟画面，真实相机会话随相机里程碑接入。
struct LocateFlowView: View {
    @Environment(ScanModel.self) private var model
    @Environment(LanguageStore.self) private var langStore
    @Environment(\.dismiss) private var dismiss
    @State private var connector: ConnectModel?
    @State private var mode: LocateMode = .sheet      // sheet / input / camera（取景模拟）
    @State private var locateInput: String = "0095690" // 预填厂商前缀（Android DEFAULT_DEV_EUI_PREFIX 同源）
    @State private var inputError: String?
    @State private var pulse = false

    enum LocateMode: Equatable {
        case sheet, input, camera
    }

    private var zh: Bool { langStore.isZh }

    var body: some View {
        ZStack {
            if model.locatePhase == .finding || model.locatePhase == .notFound {
                locatePhasePage          // 定位中/未找到：全屏页（原型 home 内 locateView）
            } else {
                sheetOrInputPage         // 弹层与输入对话框
            }
        }
        .background(Theme.bg)
        .toolbar(.hidden, for: .navigationBar)
        .onAppear {
            // 演示自动导航：定位中态直达
            if DemoLaunch.isPage("locate-finding") {
                mode = .input
                model.startLocate(devEUI: "0095690A3F2AB7C4")
            }
        }
        .fullScreenCover(item: $connector) { connector in
            ConnectOverlayView(model: connector)
                .environment(model)
                .environment(langStore)
        }
    }

    // MARK: - 定位中 / 未找到（全屏页）

    @ViewBuilder
    private var locatePhasePage: some View {
        VStack(spacing: 0) {
            NavbarHeader(title: zh ? "定位设备" : "Locate Device",
                         backText: zh ? "‹ 取消" : "‹ Cancel",
                         onBack: { model.cancelLocate(); dismiss() }) {
                EmptyView()
            }
            if model.locatePhase == .finding {
                CenterStateContent {
                    pulseCircle
                    Text(zh ? "正在寻找 …\(model.locateSuffix ?? "")" : "Searching for …\(model.locateSuffix ?? "")")
                        .font(.hkt(17, .semibold))
                    Text(zh ? "靠近目标设备可加快定位，找到后自动连接" : "Move closer to speed up locating; connects automatically once found")
                        .font(.hkt(13)).lineSpacing(1.5 * 13 - 13)
                        .foregroundStyle(Theme.text2)
                        .multilineTextAlignment(.center)
                }
                Spacer()
            } else {
                VStack {
                    CenterStateContent {
                        Text("🔍").font(.system(size: 44))
                        Text(zh ? "未找到 …\(model.locateSuffix ?? "")" : "…\(model.locateSuffix ?? "") not found")
                            .font(.hkt(17, .semibold))
                        Text(zh ? "请确认设备已上电、在信号范围内，并核对标签 DevEUI 后 6 位"
                                : "Make sure the device is powered and nearby; check the last 6 chars of the label DevEUI")
                            .font(.hkt(13)).lineSpacing(1.5 * 13 - 13)
                            .foregroundStyle(Theme.text2)
                            .multilineTextAlignment(.center)
                    }
                    .frame(minHeight: 380)
                    Spacer()
                }
            }
        }
        .background(Theme.bg)
        .toolbar(.hidden, for: .navigationBar)
        .onAppear {
            guard model.locatePhase == .finding else { return }
            withAnimation(.easeOut(duration: 1.2).repeatForever(autoreverses: true)) { pulse = true }
        }
    }

    /// .pulse：72 圆 info 16% 底 + ping 扩散环（原型 ping 动画）。
    private var pulseCircle: some View {
        ZStack {
            Circle()
                .stroke(Theme.info, lineWidth: 2)
                .frame(width: 100, height: 100)
                .scaleEffect(pulse ? 1.3 : 0.55)
                .opacity(pulse ? 0 : 0.7)
            Circle()
                .fill(Theme.info.opacity(0.16))
                .frame(width: 72, height: 72)
            Text("📡").font(.system(size: 30))
        }
        .onAppear {
            pulse = false
            withAnimation(.easeOut(duration: 1.6).repeatForever(autoreverses: false)) { pulse = true }
        }
    }

    // MARK: - sheet 弹层 / input 对话框 / camera 取景

    @ViewBuilder
    private var sheetOrInputPage: some View {
        switch mode {
        case .sheet: locateSheet
        case .input: inputDialog
        case .camera: cameraView
        }
    }

    /// .sheet 底部弹层（mask 0.35 + 圆角 20 卡）。
    private var locateSheet: some View {
        ZStack(alignment: .bottom) {
            Theme.bg.ignoresSafeArea()
            Color.black.opacity(0.35).ignoresSafeArea()
            VStack(alignment: .leading, spacing: 0) {
                Text(zh ? "定位设备" : "Locate Device")
                    .font(.hkt(16, .bold))
                    .foregroundStyle(Theme.text)
                Text(zh ? "扫描设备标签上的二维码，或输入 16 位 DevEUI，自动找到并连接目标"
                        : "Scan the QR code on the device label, or enter the 16-hex DevEUI, to find and connect the target automatically")
                    .font(.hkt(13))
                    .foregroundStyle(Theme.text2)
                    .lineSpacing(1.6 * 13 - 13)
                    .padding(.top, 4)
                sheetOption(icon: "📷", text: zh ? "扫描设备二维码" : "Scan Device QR Code") {
                    mode = .camera
                }
                sheetOption(icon: "⌨︎", text: zh ? "输入 DevEUI" : "Enter DevEUI") {
                    mode = .input
                }
                Button {
                    dismiss()
                } label: {
                    Text(zh ? "取消" : "Cancel")
                        .font(.hkt(16, .semibold))
                        .foregroundStyle(Theme.text)
                        .frame(maxWidth: .infinity)
                        .padding(13)
                        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
                        .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius)
                            .stroke(Theme.line, lineWidth: 1))
                }
                .padding(.top, 14)
            }
            .padding(16)
            .padding(.bottom, 26)
            .background(Theme.card, in: RoundedRectangle(cornerRadius: 20))
        }
        .toolbar(.hidden, for: .navigationBar)
    }

    private func sheetOption(icon: String, text: String, action: @escaping () -> Void) -> some View {
        Button(action: action) {
            HStack(spacing: 12) {
                Text(icon).font(.system(size: 20))
                Text(text).font(.hkt(15, .semibold)).foregroundStyle(Theme.text)
                Spacer()
            }
            .padding(EdgeInsets(top: 14, leading: 12, bottom: 14, trailing: 12))
            .background(Theme.card2, in: RoundedRectangle(cornerRadius: 12))
        }
        .padding(.top, 10)
    }

    /// input 对话框（16 位十六进制校验，点击时校验并提示——与确认原型一致）。
    private var inputDialog: some View {
        ZStack {
            Theme.bg.ignoresSafeArea()
            VStack(spacing: 0) {
                Text(zh ? "输入 DevEUI" : "Enter DevEUI")
                    .font(.hkt(16, .bold))
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(.bottom, 8)
                Text(zh ? "DevEUI（16 位十六进制）" : "DevEUI (16 hex chars)")
                    .font(.hkt(13))
                    .foregroundStyle(Theme.text2)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(.bottom, 6)
                TextField("", text: $locateInput, prompt: Text("0095690A3F2AB7C4")
                    .font(.system(.body, design: .monospaced)))
                    .font(.system(.body, design: .monospaced))
                    .autocorrectionDisabled()
                    .textInputAutocapitalization(.characters)
                    .onChange(of: locateInput) { _, newValue in
                        let filtered = String(newValue.filter { $0.isHexDigit }.uppercased().prefix(16))
                        if filtered != newValue { locateInput = filtered }
                    }
                    .padding(.horizontal, 12).padding(.vertical, 10)
                    .background(Theme.card2, in: RoundedRectangle(cornerRadius: 10))
                    .overlay(RoundedRectangle(cornerRadius: 10).stroke(Theme.line, lineWidth: 1))
                if let inputError {
                    Text("✕ " + inputError)
                        .font(.hkt(12)).foregroundStyle(Theme.err)
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .padding(.top, 6)
                }
                HStack(spacing: 10) {
                    DialogButton(title: zh ? "取消" : "Cancel") { mode = .sheet }
                    DialogButton(title: zh ? "定位" : "Locate", kind: .primary) {
                        let cleaned = locateInput.uppercased()
                        if cleaned.count == 16, cleaned.allSatisfy({ $0.isHexDigit }) {
                            inputError = nil
                            model.startLocate(devEUI: cleaned)   // → finding；命中 onLocateHit
                        } else {
                            inputError = zh ? "须为 16 位十六进制（0-9、A-F）" : "Must be 16 hex characters (0-9, A-F)"
                        }
                    }
                }
                .padding(.top, 14)
            }
            .padding(EdgeInsets(top: 20, leading: 18, bottom: 20, trailing: 18))
            .frame(width: 296)
            .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.dialogRadius))
        }
        .toolbar(.hidden, for: .navigationBar)
    }

    /// .cam 相机取景模拟（真实相机会话随相机里程碑接入）。
    private var cameraView: some View {
        ZStack {
            Color(hex: "#0B0B0F").ignoresSafeArea()
            VStack(spacing: 0) {
                NavbarHeader(title: zh ? "扫描设备二维码" : "Scan Device QR Code",
                             backText: zh ? "取消" : "Cancel",
                             onBack: { mode = .sheet }) {
                    EmptyView()
                }
                .tint(Color(hex: "#23ADE5"))
                Spacer()
                ZStack {
                    CamCornerFrame()
                        .frame(width: 240, height: 240)
                }
                Text(zh ? "对准设备标签上的二维码，自动识别" : "Point at the QR code on the device label to scan")
                    .font(.hkt(13))
                    .foregroundStyle(Color(hex: "#BBBBBB"))
                    .padding(.top, 26)
                Spacer()
            }
        }
        .toolbar(.hidden, for: .navigationBar)
    }
}

/// 相机取景四角框（.cam .frame：240×240、四角 34px 3px 线、圆角 8）。
struct CamCornerFrame: View {
    var body: some View {
        let half: CGFloat = (240 - 34) / 2
        ZStack {
            CamCorner()
                .offset(x: -half, y: -half)                                     // 左上
            CamCorner().rotationEffect(.degrees(90)).offset(x: half, y: -half)  // 右上
            CamCorner().rotationEffect(.degrees(-90)).offset(x: -half, y: half) // 左下
            CamCorner().rotationEffect(.degrees(180)).offset(x: half, y: half)  // 右下
        }
        .frame(width: 240, height: 240)
    }
}

/// 单个 L 形取景角（左上形状，其余旋转复用）。
struct CamCorner: View {
    var body: some View {
        Path { path in
            path.move(to: CGPoint(x: 0, y: 34))
            path.addLine(to: CGPoint(x: 0, y: 0))
            path.addLine(to: CGPoint(x: 34, y: 0))
        }
        .stroke(Color(hex: "#23ADE5"), style: StrokeStyle(lineWidth: 3, lineCap: .round))
    }
}