import CoreBLE
import CoreProtocol
import SwiftUI

/// 校准页（P_cal）—— 克隆冻结原型 + 用户 2026-09-10 三条流程裁决（规格卡 `docs/ios/design/ui-spec/P-cal.md` §5）：
/// 1. 无二次确认，点「开始校准」直接开始；
/// 2. 成功 → 显示成功信息，停顿 3 秒自动返回详情页；
/// 3. 失败 → 弹窗询问是否再次尝试，取消则返回。
/// 真实流程：0xFD 触发（设备立即 ACK）→ 设备端执行 → "Calibration Done" 文本上报驱动成功；
/// 超时（UDS 120s / DC 180s，Android 同源）走失败分支。
struct CalibrationView: View {
    let session: DeviceSession

    @Environment(LanguageStore.self) private var langStore
    @Environment(\.dismiss) private var dismiss
    @State private var state: CalState = .idle
    @State private var elapsed = 0
    @State private var showRetry = false
    @State private var timer: Timer?
    @State private var calTask: Task<Void, Never>?

    enum CalState: Equatable {
        case idle, running, success, failure
    }

    private var family: DeviceFamily { session.family }
    private var isUDS: Bool { family == .uds100 }
    private var zh: Bool { langStore.isZh }

    var body: some View {
        VStack(spacing: 0) {
            NavbarHeader(title: zh ? "校准" : "Calibration",
                         backText: zh ? "‹ 返回" : "‹ Back",
                         onBack: { stopTimer(); dismiss() }) {
                EmptyView()
            }
            // 用户 2026-09-10 裁决：等待页内容居中、取消贴底（与连接覆盖层同风格）
            if state == .running {
                VStack(spacing: 0) {
                    runningCard
                        .frame(maxWidth: .infinity, maxHeight: .infinity)
                    cancelButton
                        .padding(24)
                }
            } else {
                ScrollView {
                    VStack(alignment: .leading, spacing: 0) {
                        switch state {
                        case .idle: idleCard
                        case .success: successCenter
                        case .failure: failureCenter
                        default: EmptyView()
                        }
                    }
                    .padding(.horizontal, 16)
                    .padding(.bottom, 24)
                }
            }
        }
        .background(Theme.bg)
        .toolbar(.hidden, for: .navigationBar)
        .overlay { retryDialog }
        .onDisappear {
            stopTimer()
            calTask?.cancel()
            session.cancelCalibrationWait()   // 中途退出=放弃等待上报，设备端校准继续
        }
    }

    // MARK: - idle 指引卡片（规格卡 §1 idle）

    private var idleCard: some View {
        VStack(alignment: .leading, spacing: 12) {
            VStack(alignment: .leading, spacing: 0) {
                HStack(spacing: 6) {
                    Text("🧭")
                    Text(isUDS ? (zh ? "倾角校准" : "Tilt Calibration")
                               : (zh ? "磁力计校准" : "Magnetometer Calibration"))
                        .font(.hkt(16, .bold))
                        .foregroundStyle(Theme.text)
                }
                .padding(.bottom, 10)
                ForEach(steps.indices, id: \.self) { index in
                    let step = steps[index]
                    CalStepRow(index: index + 1,
                               title: step.title,
                               text: step.text)
                }
                HStack(alignment: .top, spacing: 0) {
                    Text("⚠︎ " + (zh ? "校准为长时操作，期间其他命令已禁用"
                                      : "Long-running operation; other commands are disabled meanwhile"))
                        .font(.hkt(13))
                        .lineSpacing(1.6 * 13 - 13)
                        .foregroundStyle(Theme.text2)
                }
                .padding(.top, 10)
                .frame(maxWidth: .infinity, alignment: .leading)
                .overlay(alignment: .top) {
                    Rectangle().fill(Theme.line).frame(height: 1)
                }
            }
            .padding(EdgeInsets(top: 13, leading: 14, bottom: 13, trailing: 14))
            .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
            .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
                .stroke(Theme.line.opacity(0.82), lineWidth: 1))
            .hktShadow()

            // 裁决 1：无二次确认，直接开始
            Button {
                begin()
            } label: {
                Text(zh ? "开始校准" : "Start Calibration")
                    .font(.hkt(16, .semibold))
                    .foregroundStyle(.white)
                    .frame(maxWidth: .infinity)
                    .padding(13)
                    .background(Theme.info, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
            }
        }
    }

    private struct GuideStep {
        let title: String?
        let text: String
    }

    private var steps: [GuideStep] {
        if isUDS {
            return [
                GuideStep(title: nil, text: zh ? "避免强磁与金属台面干扰；清理设备周围杂物" : "Avoid strong magnets and metal surfaces; clear debris around the device"),
                GuideStep(title: nil, text: zh ? "设备水平静止放置" : "Place the device level and still"),
                GuideStep(title: nil, text: zh ? "开始后保持设备完全静止，等待完成提示" : "After starting, keep the device completely still until prompted"),
            ]
        }
        return [
            GuideStep(title: zh ? "邻位车辆" : "Neighboring vehicles", text: zh ? "相邻车位 2 米内无车辆停放，理想时机为安装后车位尚空时" : "No vehicles parked within 2 m of adjacent spaces; ideally calibrate while the parking space is still empty after installation"),
            GuideStep(title: zh ? "固定铁磁结构" : "Fixed ferromagnetic structures", text: zh ? "车位 1 米内无井盖、灯柱基座、裸露钢筋、消防栓" : "No manhole covers, lamp-post bases, exposed rebar, or fire hydrants within 1 m of the parking space"),
            GuideStep(title: zh ? "通电电缆" : "Energized cables", text: zh ? "距地埋电缆、配电箱 1 米以上" : "Stay more than 1 m from buried cables and distribution boxes"),
            GuideStep(title: zh ? "随身磁物" : "Personal magnetic items", text: zh ? "磁吸手机支架、钥匙串、机械手表等距设备 0.5 米以上" : "Keep MagSafe phone mounts, keychains, mechanical watches, and similar items at least 0.5 m from the device"),
            GuideStep(title: zh ? "设备状态" : "Device status", text: zh ? "传感器已按规范水平嵌入路面，校准期间不得触碰" : "The sensor is embedded level in the road surface per specification; do not touch it during calibration"),
            GuideStep(title: zh ? "触发并等待" : "Trigger and wait", text: zh ? "指令触发后人员退开 2 米，等待自动完成，勿重复触发" : "After triggering the command, move at least 2 m away and wait for automatic completion; do not trigger repeatedly"),
            GuideStep(title: zh ? "验证与失败处理" : "Verification and failure handling", text: zh ? "完成上报后做一次停车 / 驶离验证；失败时先排查邻位车辆、井盖等干扰源，排除后再重试，勿原地反复重试" : "After completion is reported, perform one parking/departure verification; if it fails, first check interference sources such as adjacent vehicles and manhole covers, retry only after clearing them, and do not retry repeatedly in place"),
        ]
    }

    // MARK: - running 进行中（规格卡 §1 running）

    private var runningCard: some View {
        VStack(spacing: 12) {
            VStack(spacing: 6) {
                StateBadge(kind: .info, text: zh ? "校准进行中…（以设备上报为准）" : "Calibrating… (as reported by the device)")
                Text(zh ? "已用时 \(elapsed) 秒 · \(isUDS ? "预计约 90 秒，请耐心等待" : "磁力计校准耗时较长（最长约 3 分钟），请耐心等待")"
                        : "Elapsed \(elapsed)s · \(isUDS ? "Expected ~90 seconds, please wait" : "Magnetometer calibration can take up to 3 minutes")")
                    .font(.hkt(13))
                    .foregroundStyle(Theme.text2)
                    .multilineTextAlignment(.center)
                HKTProgress(fraction: Double(elapsed) / 8.0)
                    .padding(.top, 4)
            }
            .padding(13)
            .frame(maxWidth: .infinity)
            .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
            .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
                .stroke(Theme.line.opacity(0.82), lineWidth: 1))
            .hktShadow()
        }
    }

    /// 取消按钮：贴底全宽（与 P-02 取消同规格）。
    private var cancelButton: some View {
        Button {
            stopTimer()
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
    }

    // MARK: - success 成功（裁决 2：显示成功信息，停顿 3 秒自动返回）

    private var successCenter: some View {
        VStack(spacing: 10) {
            Text("✓").font(.system(size: 46)).foregroundStyle(Theme.ok)
            Text(zh ? "校准完成" : "Calibration complete").font(.hkt(17, .semibold))
        }
        .padding(.horizontal, 32)
        .frame(maxWidth: .infinity, minHeight: 380)
        .task {
            try? await Task.sleep(for: .seconds(3))
            dismiss()
        }
    }

    // MARK: - failure 失败（裁决 3：弹窗询问是否再次尝试）

    private var failureCenter: some View {
        VStack(spacing: 10) {
            Text("✕").font(.system(size: 46)).foregroundStyle(Theme.err)
            Text(zh ? "校准未成功" : "Calibration failed").font(.hkt(17, .semibold))
        }
        .padding(.horizontal, 32)
        .frame(maxWidth: .infinity, minHeight: 380)
        .task {
            guard !showRetry else { return }
            showRetry = true
        }
    }

    @ViewBuilder
    private var retryDialog: some View {
        if state == .failure, showRetry {
            HKTDialog(title: zh ? "校准未成功" : "Calibration failed",
                      message: zh ? "本次校准未成功。是否再次尝试校准？"
                                  : "This calibration attempt did not succeed. Try again?") {
                DialogButton(title: zh ? "取消" : "Cancel") {
                    stopTimer()
                    dismiss()   // 裁决 3：取消=返回详情页
                }
                DialogButton(title: zh ? "再次尝试" : "Try Again", kind: .primary) {
                    showRetry = false
                    begin()     // 重新进入进行中
                }
            }
        }
    }

    // MARK: - 状态机（真实校准：0xFD ACK → 等设备上报；计时条沿用原型 elapsed/8 视觉）

    private func begin() {
        state = .running
        elapsed = 0
        timer?.invalidate()
        timer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { _ in
            MainActor.assumeIsolated { elapsed += 1 }
        }
        calTask = Task {
            let outcome = await session.startCalibration()
            guard !Task.isCancelled else { return }
            stopTimer()
            state = outcome == .done ? .success : .failure
        }
    }

    private func stopTimer() {
        timer?.invalidate()
        timer = nil
    }
}
