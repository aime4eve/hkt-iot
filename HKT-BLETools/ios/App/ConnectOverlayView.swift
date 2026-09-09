import CoreBLE
import SwiftUI

/// P-02 连接覆盖层 —— 1:1 克隆冻结原型（规格卡 `docs/ios/design/ui-spec/P-02.md`）。
/// 全屏 --bg：🔗 + 设备名 + steps(3) + 阶段文案 + 取消（.btn secondary）；
/// 失败态原型未设计，按规格卡 §4-1 用原型按钮样式渲染原因/重试/返回。
struct ConnectOverlayView: View {
    let model: ConnectModel
    @Environment(\.dismiss) private var dismiss
    @Environment(ScanModel.self) private var scanModel
    @Environment(LanguageStore.self) private var langStore
    @State private var dismissed = false

    private var zh: Bool { langStore.isZh }

    var body: some View {
        VStack(spacing: 0) {
            if model.outcome == nil {
                CenterStateContent {
                    connectingCenter
                }
                cancelArea
            } else if case .failed = model.outcome {
                CenterStateContent {
                    failedCenter
                }
                failedActions
            }
        }
        .background(Theme.bg)
        .onAppear { model.start() }   // 关键：弹出即启动三阶段连接（此前从未调用，连接永远不发）
        .onChange(of: model.outcome) { _, outcome in
            guard let outcome, !dismissed else { return }
            switch outcome {
            case .connected:
                LogStore.shared.info(zh ? "已连接 \(model.target.name)" : "Connected \(model.target.name)")
                scanModel.residentDevice = ResidentDevice(
                    name: model.target.name,
                    identifier: model.target.identifier)
                scanModel.makeAndStartSession(for: model.target)   // R-5：连接成功即启动会话轮询
                scanModel.requestShowDetail = true                 // 覆盖层关闭后自动进入详情页
                dismissed = true
                dismiss()
            case .cancelled:
                // 用户主动取消：直接关闭覆盖层回扫描页（不做驻留）
                LogStore.shared.info(zh ? "连接已取消" : "Connect cancelled")
                dismissed = true
                dismiss()
            case .failed(let failure):
                LogStore.shared.error(zh ? "连接失败：\(model.failureText ?? "\(failure)")" : "Connect failed: \(failure)")
            }
        }
    }

    // MARK: - 连接中（规格卡 §1 DOM）

    private var connectingCenter: some View {
        Group {
            Text("🔗").font(.system(size: 44))
            Text("\(model.target.name) …\(idSuffix)")
                .font(.hkt(17, .semibold))
                .foregroundStyle(Theme.text)
            Steps(total: 3, onCount: min(model.phaseIndex + 1, 3))
            Text(phaseText)
                .font(.hkt(13))
                .lineSpacing(1.5 * 13 - 13)
                .foregroundStyle(Theme.text2)
                .multilineTextAlignment(.center)
        }
    }

    private var phaseText: String {
        let phases: [String] = zh
            ? ["正在连接", "正在发现服务…", "正在订阅通知…"]
            : ["Connecting", "Discovering services…", "Subscribing to notifications…"]
        return phases[min(model.phaseIndex, 2)]
    }

    /// 取消（.btn secondary 全宽，容器 padding 24；cancelConn=终止并回扫描页）。
    private var cancelArea: some View {
        Button {
            model.cancel()
            dismissed = true
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
        .padding(24)
    }

    // MARK: - 失败态（原型未设计，规格卡 §4-1）

    private var failedCenter: some View {
        Group {
            Text("⚠️").font(.system(size: 44))
            Text("\(model.target.name) …\(idSuffix)")
                .font(.hkt(17, .semibold))
                .foregroundStyle(Theme.text)
            Text((zh ? "连接失败：" : "Connection failed: ") + (model.failureText ?? (zh ? "未知原因" : "Unknown")))
                .font(.hkt(13))
                .lineSpacing(1.5 * 13 - 13)
                .foregroundStyle(Theme.err)
                .multilineTextAlignment(.center)
        }
    }

    /// 重新连接（.btn primary）+ 返回（.btn secondary）。
    private var failedActions: some View {
        VStack(spacing: 10) {
            Button {
                model.retry()
            } label: {
                Text(zh ? "重新连接" : "Retry")
                    .font(.hkt(16, .semibold))
                    .foregroundStyle(.white)
                    .frame(maxWidth: .infinity)
                    .padding(13)
                    .background(Theme.info, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
            }
            Button {
                dismissed = true
                dismiss()
            } label: {
                Text(zh ? "返回" : "Back")
                    .font(.hkt(16, .semibold))
                    .foregroundStyle(Theme.text)
                    .frame(maxWidth: .infinity)
                    .padding(13)
                    .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
                    .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius)
                        .stroke(Theme.line, lineWidth: 1))
            }
        }
        .padding(24)
    }

    private var idSuffix: String {
        String(model.target.identifier.uuidString.suffix(4)).uppercased()
    }
}

/// .center 布局（gap 10、水平 padding 32、弹性居中）——P-02 覆盖层内部用。
private struct CenterStateContent<Content: View>: View {
    @ViewBuilder var content: Content

    var body: some View {
        VStack(spacing: 10) {
            content
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .padding(.horizontal, 32)
    }
}
