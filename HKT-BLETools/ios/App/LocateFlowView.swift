import CoreBLE
import SwiftUI

/// R-2 目标设备定位流程：输入 DevEUI → 定位中（脉冲动画）→ 命中自动连接 / 未找到重试。
/// 扫码（相机）入口按钮已就位，相机会话随 M4 相机里程碑接入。
struct LocateFlowView: View {
    @Environment(ScanModel.self) private var model
    @Environment(\.dismiss) private var dismiss
    @State private var connector: ConnectModel?
    @State private var locateInput: String = "0095690"   // 预填厂商前缀（Android DEFAULT_DEV_EUI_PREFIX 同源）
    @State private var pulse = false
    @State private var inputError: String?

    private var zh: Bool { Locale.current.language.languageCode?.identifier == "zh" }

    var body: some View {
        NavigationStack {
            content
                .navigationTitle(zh ? "定位设备" : "Locate Device")
                .navigationBarTitleDisplayMode(.inline)
                .toolbar {
                    ToolbarItem(placement: .topBarLeading) {
                        Button(zh ? "关闭" : "Close") { model.cancelLocate(); dismiss() }
                    }
                }
        }
        .background(Theme.bg)
        .fullScreenCover(item: $connector) { connector in
            ConnectOverlayView(model: connector)
        }
    }

    @ViewBuilder
    private var content: some View {
        switch model.locatePhase {
        case .input, .none:
            inputView
        case .finding:
            findingView
        case .notFound:
            notFoundView
        }
    }

    // MARK: - 输入

    private var inputView: some View {
        VStack(alignment: .leading, spacing: 14) {
            Text(zh ? "输入设备的 16 位 DevEUI，定位后自动连接" : "Enter the device's 16-hex DevEUI to locate and connect automatically")
                .font(.subheadline).foregroundStyle(Theme.text2)
            HStack(spacing: 0) {
                Text("0095690").font(.system(.body, design: .monospaced)).foregroundStyle(Theme.text2)
                TextField("", text: $locateInput, prompt: Text("A3F2AB7C4").font(.system(.body, design: .monospaced)))
                    .font(.system(.body, design: .monospaced))
                    .autocorrectionDisabled()
                    .textInputAutocapitalization(.characters)
                    .onChange(of: locateInput) { _, newValue in
                        let filtered = String(newValue.filter { $0.isHexDigit }.uppercased().prefix(16))
                        if filtered != newValue { locateInput = filtered }
                    }
            }
            .padding(10)
            .background(Theme.card2, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
            .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius).stroke(Theme.line, lineWidth: 1))
            if let error = inputError {
                Text(error).font(.caption2).foregroundStyle(Theme.err)
            }
            Button {
                // 与确认原型一致：按钮始终可点，点击时校验并提示（1:1）
                let cleaned = locateInput.uppercased()
                if cleaned.count == 16, cleaned.allSatisfy({ $0.isHexDigit }) {
                    inputError = nil
                    model.startLocate(devEUI: cleaned)
                } else {
                    inputError = zh ? "须为 16 位十六进制（0-9、A-F）" : "Must be 16 hex characters (0-9, A-F)"
                }
            } label: {
                Text(zh ? "开始定位" : "Start Locating")
                    .frame(maxWidth: .infinity).frame(height: 44)
            }
            .buttonStyle(.borderedProminent)
            Button {
                model.beginCameraLocate()   // 相机会话随相机里程碑接入
            } label: {
                HStack(spacing: 6) {
                    Image(systemName: "qrcode.viewfinder")
                    Text(zh ? "扫描设备二维码（后续接入）" : "Scan QR code (coming)")
                }
                .frame(maxWidth: .infinity).frame(height: 44)
            }
            .buttonStyle(.bordered)
            .disabled(true)
            Spacer()
        }
        .padding(16)
    }

    // MARK: - 定位中

    private var findingView: some View {
        VStack(spacing: 14) {
            Spacer()
            ZStack {
                Circle().stroke(Theme.info.opacity(0.18), lineWidth: 2).frame(width: 110, height: 110)
                Circle().stroke(Theme.info.opacity(0.10), lineWidth: 2).frame(width: 150, height: 150)
                Image(systemName: "antenna.radiowaves.left.and.right")
                    .font(.system(size: 36)).foregroundStyle(Theme.info)
            }
            Text(zh ? "正在定位 …\(model.locateSuffix ?? "")" : "Locating …\(model.locateSuffix ?? "")")
                .font(.headline)
            Text(zh ? "靠近目标设备可加快定位；命中后自动连接" : "Move closer to speed up; connects automatically on match")
                .font(.subheadline).foregroundStyle(Theme.text2).multilineTextAlignment(.center)
            if !model.isReady {
                Text(zh ? "蓝牙未就绪：请在系统设置中允许蓝牙权限" : "Bluetooth not ready: allow Bluetooth permission in Settings")
                    .font(.caption).fontWeight(.semibold).foregroundStyle(Theme.err)
                    .multilineTextAlignment(.center)
            }
            Spacer()
            Button(zh ? "取消定位" : "Cancel Locating", role: .destructive) {
                model.cancelLocate()
            }
            .frame(maxWidth: .infinity).frame(height: 44)
            .buttonStyle(.bordered)
            .padding(.bottom, 20)
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .onAppear { withAnimation(.easeOut(duration: 1.2).repeatForever(autoreverses: true)) { pulse = true } }
    }

    // MARK: - 未找到

    private var notFoundView: some View {
        VStack(spacing: 14) {
            Spacer()
            Image(systemName: "magnifyingglass").font(.system(size: 44)).foregroundStyle(Theme.text2)
            Text(zh ? "未找到 …\(model.locateSuffix ?? "")" : "…\(model.locateSuffix ?? "") not found").font(.headline)
            Text(zh ? "请确认设备已上电、在信号范围内，并核对标签 DevEUI 后 6 位" : "Check power/range and the last 6 chars of the label DevEUI")
                .font(.subheadline).foregroundStyle(Theme.text2).multilineTextAlignment(.center)
            Button(zh ? "重新定位" : "Retry") { model.retryLocate() }
                .buttonStyle(.borderedProminent).frame(maxWidth: .infinity)
            Button(zh ? "返回" : "Back") { model.cancelLocate(); dismiss() }
                .buttonStyle(.bordered).frame(maxWidth: .infinity)
            Spacer()
        }
        .padding(16)
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
}
