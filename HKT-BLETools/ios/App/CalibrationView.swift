import CoreBLE
import CoreProtocol
import SwiftUI

/// 校准页（P_cal）—— 1:1 克隆冻结原型（规格卡 `docs/ios/design/ui-spec/P-cal.md`）。
/// 三态：指引卡片 →（确认对话框）→ 进行中（计时+进度条）→ 完成态。
/// 原型演示为本地 8s 计时；真实校准命令（以设备上报为准）随协议里程碑接入。
struct CalibrationView: View {
    let family: DeviceFamily

    @Environment(LanguageStore.self) private var langStore
    @Environment(\.dismiss) private var dismiss
    @State private var confirmVisible = false
    @State private var state: CalState = .idle
    @State private var elapsed = 0
    @State private var timer: Timer?

    enum CalState: Equatable {
        case idle, running, done
    }

    private var isUDS: Bool { family == .uds100 }
    private var zh: Bool { langStore.isZh }

    var body: some View {
        VStack(spacing: 0) {
            NavbarHeader(title: zh ? "校准" : "Calibration",
                         backText: zh ? "‹ 返回" : "‹ Back",
                         onBack: { stopTimer(); dismiss() }) {
                EmptyView()
            }
            ScrollView {
                VStack(alignment: .leading, spacing: 0) {
                    switch state {
                    case .idle: idleCard
                    case .running: runningCard
                    case .done: doneCenter
                    }
                }
                .padding(.horizontal, 16)
                .padding(.bottom, 24)
            }
        }
        .background(Theme.bg)
        .toolbar(.hidden, for: .navigationBar)
        .overlay { confirmDialog }
        .onDisappear { stopTimer() }
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

            Button {
                confirmVisible = true
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
    }

    // MARK: - done 完成（规格卡 §1 done）

    private var doneCenter: some View {
        VStack(spacing: 10) {
            Text("✓").font(.system(size: 46)).foregroundStyle(Theme.ok)
            Text(zh ? "✓ 校准完成" : "✓ Calibration complete").font(.hkt(17, .semibold))
            Button {
                dismiss()
            } label: {
                Text(zh ? "完成" : "Done")
                    .font(.hkt(16, .semibold))
                    .foregroundStyle(.white)
                    .padding(.vertical, 12).padding(.horizontal, 40)
                    .background(Theme.info, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
            }
            .padding(.top, 2)
        }
        .padding(.horizontal, 32)
        .frame(maxWidth: .infinity, minHeight: 380)
    }

    // MARK: - 确认对话框（规格卡 §1）

    @ViewBuilder
    private var confirmDialog: some View {
        if confirmVisible {
            HKTDialog(title: zh ? "校准" : "Calibration",
                      message: zh ? (isUDS ? "将设备水平静止放置，然后开始校准" : "请确认 7 项环境与操作要求后开始校准")
                                  : (isUDS ? "Place the device level and still, then start" : "Confirm the seven environment and operation checks, then start"),
                      buttons: {
                DialogButton(title: zh ? "取消" : "Cancel") { confirmVisible = false }
                DialogButton(title: zh ? "开始校准" : "Start Calibration", kind: .primary) {
                    confirmVisible = false
                    begin()
                }
            })
        }
    }

    // MARK: - 状态机（原型 8s 演示周期；真实校准命令随协议里程碑接入）

    private func begin() {
        state = .running
        elapsed = 0
        timer?.invalidate()
        timer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { _ in
            MainActor.assumeIsolated {
                elapsed += 1
                if elapsed >= 8 {
                    stopTimer()
                    state = .done
                }
            }
        }
    }

    private func stopTimer() {
        timer?.invalidate()
        timer = nil
    }
}
