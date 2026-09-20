import CoreBLE
import SwiftUI
import UIKit

/// R-2 目标设备定位 UI —— 1:1 克隆冻结原型（定位 sheet / 输入对话框 / 相机取景 / 定位中脉冲 / 未找到）。
/// 状态机 = ScanModel.locatePhase（input/finding/notFound，30s 超时未找到，命中 onLocateHit→连接覆盖层）。
/// 相机=AVFoundation 真会话（CameraScanner，2026-09-14 接入）：识别文本须 16 位 hex DevEUI（Android 同规）。
struct LocateFlowView: View {
    @Environment(ScanModel.self) private var model
    @Environment(LanguageStore.self) private var langStore
    @Environment(\.dismiss) private var dismiss
    @State private var connector: ConnectModel?
    @State private var mode: LocateMode = .sheet      // sheet / input / camera（真会话）
    @State private var locateInput: String = "0095690" // 预填厂商前缀（Android DEFAULT_DEV_EUI_PREFIX 同源）
    @State private var inputError: String?
    @State private var pulse = false
    @State private var camState: CameraScanner.RunState = .running
    @State private var qrHint: String?
    @State private var pendingTarget: DiscoveredDevice?

    enum LocateMode: Equatable {
        case sheet, input, camera
    }

    private var zh: Bool { langStore.isZh }

    var body: some View {
        ZStack {
            if let connector {
                // 连接覆盖层内嵌于定位层（同一弹层槽位内切换）。勿改回 fullScreenCover 嵌套：
                // 定位弹层上再叠全屏弹层会触发 SwiftUI 观察失效——模型已 connected 而界面永远停在
                // 「正在连接」第一格（2026-09-15 真机取证：事件全达、onChange 不触发）
                ConnectOverlayView(model: connector)
            } else if model.locatePhase == .finding || model.locatePhase == .notFound {
                locatePhasePage          // 定位中/未找到：全屏页（原型 home 内 locateView）
            } else {
                sheetOrInputPage         // 弹层与输入对话框
            }
        }
        .background(Theme.bg)
        .toolbar(.hidden, for: .navigationBar)
        // 定位命中：由本定位层自己渲染连接覆盖层（内嵌，见 body 注释）。此前由扫描页接管，
        // 两个全屏弹层竞争宿主导致覆盖层吞掉定位层、详情页推入也被吞（2026-09-15 真机缺陷）
        .onChange(of: model.locateHitDevice) { _, hit in
            guard let hit else { return }
            model.locateHitDevice = nil   // 消费掉：同一设备再次命中仍需触发
            // 已有驻留会话 → R-32 切换确认（用户裁决 2026-09-20）：是=断开重连；否=维持现状退出定位。
            // 不做此防护会在旧会话未释放时连新设备：底层链路被新设备占据，旧会话按自己的协议解析
            // 新设备应答 → 详情页报「响应数据异常（未知类型）」且进入错误详情页
            if let resident = model.residentDevice, model.activeSession != nil {
                if resident.identifier == hit.identifier {
                    model.requestShowDetail = true   // 定位到的就是当前设备：直接回详情
                    dismiss()
                } else {
                    pendingTarget = hit
                }
                return
            }
            connector = model.connector(for: hit)
        }
        // 连接成功（覆盖层置位 requestShowDetail）：扫描页推详情页，本定位层随覆盖层 dismiss 退场
        .onChange(of: model.requestShowDetail) { _, request in
            if request { dismiss() }
        }
        // R-32 切换确认框（R-31 释放语义：取消=维持现状退出定位，确认=断开旧会话连接目标）
        .overlay {
            if let target = pendingTarget, let current = model.residentDevice {
                HKTDialog(title: zh ? "切换设备？" : "Switch device?",
                          message: Text(zh ? "当前已连接 " : "Currently connected to ")
                              + Text(current.name).bold()
                              + Text(zh ? "。切换将断开当前会话并连接 " : ". Switching will disconnect it and connect to ")
                              + Text(target.name).bold()
                              + Text(zh ? "。" : "."),
                          buttons: {
                    DialogButton(title: zh ? "取消" : "Cancel") {
                        pendingTarget = nil
                        dismiss()   // 维持现状：退出定位流程，原连接保持不动
                    }
                    DialogButton(title: zh ? "切换并连接" : "Switch & Connect", kind: .primary) {
                        pendingTarget = nil
                        model.disconnectActive()
                        connector = model.connector(for: target)
                    }
                })
            }
        }
        .onAppear {
            // 演示自动导航：定位中态直达
            if DemoLaunch.isPage("locate-finding") {
                mode = .input
                model.startLocate(devEUI: "0095690A3F2AB7C4")
            }
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

    /// .cam 相机取景真会话：识别命中→startLocate（body 随 locatePhase 切到定位中页）。
    private var cameraView: some View {
        ZStack {
            Color(hex: "#0B0B0F").ignoresSafeArea()
            CameraScanner(onCode: handleQR, state: $camState)
                .ignoresSafeArea()
                .opacity(camState == .running ? 1 : 0)
            VStack(spacing: 0) {
                NavbarHeader(title: zh ? "扫描设备二维码" : "Scan Device QR Code",
                             backText: zh ? "取消" : "Cancel",
                             onBack: { mode = .sheet }) {
                    EmptyView()
                }
                .tint(Color(hex: "#23ADE5"))
                Spacer()
                if camState == .running {
                    ZStack {
                        CamCornerFrame()
                            .frame(width: 240, height: 240)
                    }
                    Text(zh ? "对准设备标签上的二维码，自动识别" : "Point at the QR code on the device label to scan")
                        .font(.hkt(13))
                        .foregroundStyle(Color(hex: "#BBBBBB"))
                        .padding(.top, 26)
                    if let qrHint {
                        Text("✕ " + qrHint)
                            .font(.hkt(12))
                            .foregroundStyle(Color(hex: "#FF6B6B"))
                            .multilineTextAlignment(.center)
                            .padding(.horizontal, 24)
                            .padding(.top, 10)
                    }
                } else {
                    cameraFallback
                }
                Spacer()
            }
        }
        .toolbar(.hidden, for: .navigationBar)
    }

    /// 权限拒绝引导 / 无相机（模拟器）兜底。
    private var cameraFallback: some View {
        VStack(spacing: 14) {
            Text(camState == .denied ? "📷" : "⚠️").font(.system(size: 44))
            Text(camState == .denied ? (zh ? "相机权限未开启" : "Camera access is off")
                                     : (zh ? "没有可用相机" : "No camera available"))
                .font(.hkt(17, .semibold))
            Text(camState == .denied
                 ? (zh ? "请在 系统设置 › HKT BLETools › 允许「相机」后返回重试"
                       : "Allow the camera in Settings › HKT BLETools, then come back")
                 : (zh ? "此设备无法扫码，可返回使用「输入 DevEUI」"
                       : "This device has no camera; use Enter DevEUI instead"))
                .font(.hkt(13)).lineSpacing(1.5 * 13 - 13)
                .foregroundStyle(Color(hex: "#BBBBBB"))
                .multilineTextAlignment(.center)
                .padding(.horizontal, 32)
            if camState == .denied {
                Button {
                    if let url = URL(string: UIApplication.openSettingsURLString) {
                        UIApplication.shared.open(url)
                    }
                } label: {
                    Text(zh ? "打开设置" : "Open Settings")
                        .font(.hkt(15, .semibold)).foregroundStyle(Color(hex: "#23ADE5"))
                        .padding(.horizontal, 28).padding(.vertical, 11)
                        .background(RoundedRectangle(cornerRadius: Theme.controlRadius)
                            .stroke(Color(hex: "#23ADE5"), lineWidth: 1))
                }
            }
        }
    }

    /// 识别结果处理：16 位 hex 才进定位（Android isValidDevEui 同规），否则提示并继续扫。
    private func handleQR(_ text: String) {
        let cleaned = text.trimmingCharacters(in: .whitespacesAndNewlines).uppercased()
        if cleaned.count == 16, cleaned.allSatisfy({ $0.isHexDigit }) {
            qrHint = nil
            model.startLocate(devEUI: cleaned)
        } else {
            let short = text.count > 40 ? String(text.prefix(40)) + "…" : text
            qrHint = zh ? "二维码内容不是 16 位 DevEUI：\(short)"
                        : "Not a 16-hex DevEUI: \(short)"
        }
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