import CoreBLE
import CoreProtocol
import SwiftUI

/// 阀门任务本地镜像存储（原型 S.tasks）：0x04/0x05 的 App 侧记录，内存态。
@MainActor
@Observable
final class ValveTaskStore {
    static let shared = ValveTaskStore()

    struct Task: Identifiable, Equatable {
        var id: Int          // 槽位 1–16
        var valve: Int       // 0 双阀 / 1 阀1 / 2 阀2
        var state: Int       // 1 开 / 0 关
        var pulse: Int
        var sh: Int, sm: Int, eh: Int, em: Int
        var days: [Bool]     // 周一…周日
    }

    private(set) var tasks: [Task] = []

    func upsert(_ task: Task) {
        if let index = tasks.firstIndex(where: { $0.id == task.id }) {
            tasks[index] = task
        } else {
            tasks.append(task)
        }
        tasks.sort { $0.id < $1.id }
    }

    func delete(_ id: Int) {
        tasks.removeAll { $0.id == id }
    }

    func deleteAll() {
        tasks.removeAll()
    }

    func isUsed(_ id: Int) -> Bool {
        tasks.contains { $0.id == id }
    }
}

/// 阀门任务页（P_tasks）—— 1:1 克隆冻结原型（规格卡 `docs/ios/design/ui-spec/P-tasks.md`）。
/// 列表：实时任务卡(0x03) + 定时任务镜像表(0x04/0x05)；编辑页：槽位/阀门/动作/脉冲/时间/重复。
/// 真实收发：设备 ACK 驱动（0x03 设备忙时静默忽略=超时，显示 busy 横幅；镜像只在确认后记录）。
struct TasksView: View {
    let session: DeviceSession

    @Environment(LanguageStore.self) private var langStore
    @Environment(\.dismiss) private var dismiss
    @State private var path: [Int] = []          // [0] = 编辑页
    @State private var savePending = false

    /// 实时开关反馈状态机（语义对照固件 control_center.c:262：
    /// 执行=保持命令状态至时长到/脉冲满，结束设备自动反向复位且无上报；仅定时任务执行中才忙）
    struct RTExec: Equatable {
        enum Phase: Equatable { case sending, executing, done }
        var phase: Phase
        var valve: Int        // 0 双阀 / 1 阀1 / 2 阀2
        var state: Int        // 1 开 / 0 关
        var dur: Int
        var pulse: Int
        var remain: Int
        var flash: String?
    }
    @State private var rt: RTExec?
    @State private var countdownTask: Task<Void, Never>?
    @State private var advOpen = false
    @State private var rtDur = "60"              // 高级：定时回位秒数（60s 后设备自动复位）
    @State private var rtPulse = "100"           // 高级：脉冲数（计满自动停止）
    @State private var busyVisible = false

    // 编辑页草稿
    @State private var editDraft = ValveTaskStore.Task(
        id: 1, valve: 1, state: 1, pulse: 100,
        sh: 8, sm: 0, eh: 18, em: 30,
        days: [true, true, true, true, true, false, false])
    @State private var validationMessage: String?
    @State private var editPulse = "100"

    private var zh: Bool { langStore.isZh }
    private let store = ValveTaskStore.shared
    private var snapshot: DeviceSnapshot { session.snapshot }

    private let dayNames = ["一", "二", "三", "四", "五", "六", "日"]   // 用户 2026-09-10 裁决：中文单字
    private let dayNamesEn = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]

    var body: some View {
        VStack(spacing: 0) {
            if path.isEmpty { listPage } else { editPage }
        }
        .background(Theme.bg)
        .toolbar(.hidden, for: .navigationBar)
        .onDisappear {
            countdownTask?.cancel()   // 页面退出：本地倒计时镜像随之失效（设备侧继续，无需取消命令）
        }
        .onAppear {
            // 演示自动导航（-demo-page task-edit）：直接进入编辑页
            if DemoLaunch.isPage("task-edit") {
                newTask()
                path = [0]
            }
        }
    }

    // MARK: - 列表页（规格卡 §1）

    private var listPage: some View {
        VStack(spacing: 0) {
            NavbarHeader(title: zh ? "阀门任务" : "Valve Tasks",
                         backText: zh ? "‹ 返回" : "‹ Back",
                         onBack: { dismiss() }) {
                LinkButton(title: (zh ? "＋ " : "＋ ") + (zh ? "新建任务" : "New Task")) {
                    newTask()
                    path = [0]
                }
            }
            ScrollView {
                VStack(alignment: .leading, spacing: 0) {
                    rtCard
                    SectionHeader(title: (zh ? "定时任务表（1–16）" : "Schedule Table (1–16)") + " · 0x04 / 0x05")
                    HKTBanner(kind: .info,
                              text: "ℹ︎ " + (zh ? "设备无 BLE 回读命令，以下为本机写入记录的镜像（固件仅支持 LoRa 平台侧回读 0x3D）"
                                                 : "No BLE read-back; this list mirrors what was written from this phone (firmware exposes 0x3D via LoRa only)"))
                    taskList
                    if !store.tasks.isEmpty {
                        Button {
                            deleteAll()
                        } label: {
                            Text(zh ? "全部删除 (0xFF)" : "Delete All (0xFF)")
                                .font(.hkt(16, .semibold))
                                .foregroundStyle(Theme.err)
                                .frame(maxWidth: .infinity)
                                .padding(13)
                                .background(Theme.err.opacity(0.10), in: RoundedRectangle(cornerRadius: Theme.controlRadius))
                                .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius)
                                    .stroke(Theme.err.opacity(0.22), lineWidth: 1))
                        }
                        Text(zh ? "删除执行中的任务将强制停止阀门动作" : "Deleting a running task force-stops the valve")
                            .font(.hkt(13))
                            .foregroundStyle(Theme.text2)
                            .frame(maxWidth: .infinity, alignment: .center)
                            .padding(.top, 6)
                    }
                }
                .padding(.horizontal, 16)
                .padding(.bottom, 24)
            }
        }
    }

    /// 实时开关卡（0x03）——每路阀一行「开阀/关阀」大按钮点按即发；当前状态侧高亮（轮询快照）。
    @ViewBuilder
    private var rtCard: some View {
        let v1 = (snapshot.valve1State ?? 0) == 1
        let v2 = (snapshot.valve2State ?? 0) == 1
        VStack(alignment: .leading, spacing: 0) {
            Text((zh ? "实时任务" : "Realtime Task") + " · 0x03")
                .font(.hkt(15, .bold))
                .foregroundStyle(Theme.text)
            Text(zh ? "立即开关阀门，点按即执行" : "Instant valve open/close - tap to actuate")
                .font(.hkt(11))
                .foregroundStyle(Theme.text2)
                .padding(.top, 2)
                .padding(.bottom, 4)
            valveRow(1, name: zh ? "阀 1" : "Valve 1",
                     cur: (zh ? "当前 " : "Now ") + (v1 ? (zh ? "开阀" : "open") : (zh ? "关阀" : "closed")), v1: v1, v2: v2)
            valveRow(2, name: zh ? "阀 2" : "Valve 2",
                     cur: (zh ? "当前 " : "Now ") + (v2 ? (zh ? "开阀" : "open") : (zh ? "关阀" : "closed")), v1: v1, v2: v2)
            valveRow(0, name: zh ? "双阀" : "Both",
                     cur: "阀1 " + (v1 ? (zh ? "开阀" : "open") : (zh ? "关阀" : "closed")) + " · 阀2 "
                          + (v2 ? (zh ? "开阀" : "open") : (zh ? "关阀" : "closed")), v1: v1, v2: v2,
                     isLast: true)
            advancedFold
            feedbackStrip
        }
        .padding(EdgeInsets(top: 13, leading: 14, bottom: 13, trailing: 14))
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
            .stroke(Theme.line.opacity(0.82), lineWidth: 1))
        .hktShadow()
        .padding(.bottom, 9)
    }

    /// 阀行：名称+当前状态 | 开阀/关阀 一对大按钮（当前状态侧 info 高亮；发送中锁定）。
    private func valveRow(_ valve: Int, name: String, cur: String, v1: Bool, v2: Bool, isLast: Bool = false) -> some View {
        HStack(spacing: 9) {
            VStack(alignment: .leading, spacing: 2) {
                Text(name).font(.hkt(14, .semibold)).foregroundStyle(Theme.text)
                Text(cur).font(.hkt(11)).foregroundStyle(Theme.text2)
            }
            Spacer(minLength: 8)
            HStack(spacing: 6) {
                rowButton(valve, state: 1, active: valve == 0 ? (v1 && v2) : (valve == 1 ? v1 : v2))
                rowButton(valve, state: 0, active: valve == 0 ? (!v1 && !v2) : (valve == 1 ? !v1 : !v2))
            }
            .frame(width: 186)
        }
        .padding(.vertical, 11)
        .overlay(alignment: .top) {
            if !isLast { Rectangle().fill(Theme.line.opacity(0.6)).frame(height: 1) }
        }
    }

    private func rowButton(_ valve: Int, state: Int, active: Bool) -> some View {
        let sending = rt?.phase == .sending
        return Button {
            sendExec(valve, state)
        } label: {
            Text(state == 1 ? (zh ? "开阀" : "Open") : (zh ? "关阀" : "Close"))
                .font(.hkt(13, .semibold))
                .foregroundStyle(active ? .white : Theme.text2)
                .frame(maxWidth: .infinity)
                .padding(.vertical, 10)
                .background(active ? Theme.info : Theme.card2,
                            in: RoundedRectangle(cornerRadius: 8))
                .opacity(sending ? 0.5 : 1)
        }
        .disabled(sending)
    }

    /// 高级折叠：定时回位时长 + 脉冲数（默认 0/0 = 纯开关保持）。
    @ViewBuilder
    private var advancedFold: some View {
        Button {
            withAnimation(.easeInOut(duration: 0.15)) { advOpen.toggle() }
        } label: {
            Text((advOpen ? "▾ " : "▸ ") + (zh ? "高级：定时回位与脉冲" : "Advanced: timed revert & pulses"))
                .font(.hkt(13, .semibold))
                .foregroundStyle(Theme.info)
        }
        .padding(.top, 9)
        if advOpen {
            HStack(spacing: 9) {
                fieldCard(label: (zh ? "持续时间" : "Duration") + "(s)") {
                    TextField("", text: $rtDur).keyboardType(.numberPad)
                    Text(zh ? "动作保持 N 秒后自动复位（0 = 保持直至下一命令）"
                            : "Hold for N seconds then auto-revert (0 = hold until the next command)")
                        .font(.hkt(11)).foregroundStyle(Theme.text2)
                }
                fieldCard(label: zh ? "脉冲数" : "Pulse count") {
                    TextField("", text: $rtPulse).keyboardType(.numberPad)
                    Text(zh ? "脉冲端口：发出 N 个脉冲后自动停止"
                            : "PWM ports: stops automatically after N pulses")
                        .font(.hkt(11)).foregroundStyle(Theme.text2)
                }
            }
            .padding(.top, 9)
        }
    }

    /// 反馈条（四态：发送中/执行中/完成闪条/忙）。
    @ViewBuilder
    private var feedbackStrip: some View {
        let cmd = rt
        if cmd?.phase == .sending {
            HKTBanner(kind: .info, text: "⏳ " + (zh ? "发送中…" : "Sending…"))
                .padding(.top, 10)
        } else if let cmd, cmd.phase == .executing {
            let name = valveName(cmd.valve)
            let action = cmd.state == 1 ? (zh ? "开阀" : "open") : (zh ? "关阀" : "close")
            HKTBanner(kind: .info,
                      text: "● " + (zh ? "设备执行中" : "Device executing")
                          + " — \(name) · \(action)" + (cmd.pulse > 0 ? " · " + (zh ? "脉冲数 " : "pulses ") + "\(cmd.pulse)" : ""))
                .padding(.top, 10)
            if cmd.dur > 0 {
                HKTProgress(fraction: cmd.dur == 0 ? 0 : Double(cmd.dur - cmd.remain) / Double(cmd.dur))
                    .padding(.top, 8)
                Text((zh ? "剩余 \(cmd.remain) s / \(cmd.dur) s · " : "\(cmd.remain)s / \(cmd.dur)s left · ")
                    + (zh ? "结束后设备自动复位阀门" : "the device restores the valve when it ends"))
                    .font(.hkt(12)).monospacedDigit().foregroundStyle(Theme.text2)
                    .padding(.top, 4)
            } else {
                Text(cmd.pulse > 0
                     ? (zh ? "执行中：设备达到脉冲数后自动结束并复位阀门"
                           : "Executing: the device stops and restores once the pulse count is reached")
                     : (zh ? "无自动结束：持续执行，直至下发新命令（新命令立即覆盖当前动作）"
                           : "No auto-stop: runs until a new command replaces it (replacement actuates immediately)"))
                    .font(.hkt(11)).foregroundStyle(Theme.text2)
                    .padding(.top, 6)
            }
        } else if let cmd, cmd.phase == .done {
            HKTBanner(kind: .ok, text: cmd.flash ?? (zh ? "✓ 任务完成 · 阀门已自动复位" : "✓ Task finished · valve restored"))
                .padding(.top, 10)
        } else if busyVisible {
            HKTBanner(kind: .err,
                      text: "⏱ " + (zh ? "设备忙（本地定时任务执行中），实时任务被静默忽略（无 ACK）"
                                        : "Device busy (schedule running); realtime task silently ignored (no ACK)"))
                .padding(.top, 10)
        }
    }

    private func valveName(_ valve: Int) -> String {
        valve == 0 ? (zh ? "双阀" : "Both") : valve == 1 ? (zh ? "阀 1" : "Valve 1") : (zh ? "阀 2" : "Valve 2")
    }

    /// 输入小卡（.field 内嵌）。
    private func fieldCard<Input: View>(label: String, @ViewBuilder input: () -> Input) -> some View {
        VStack(alignment: .leading, spacing: 0) {
            Text(label).font(.hkt(11, .semibold)).foregroundStyle(Theme.text2)
            input()
                .font(.hkt(14))
                .foregroundStyle(Theme.text)
                .padding(.top, 4)
        }
        .padding(10)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
            .stroke(Theme.line.opacity(0.82), lineWidth: 1))
    }

    @ViewBuilder
    private var taskList: some View {
        if store.tasks.isEmpty {
            Text(zh ? "暂无定时任务" : "No scheduled tasks")
                .font(.hkt(13))
                .foregroundStyle(Theme.text2)
                .frame(maxWidth: .infinity)
                .padding(.vertical, 24)
                .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
                .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
                    .stroke(Theme.line.opacity(0.82), lineWidth: 1))
                .padding(.bottom, 9)
        } else {
            ForEach(store.tasks) { task in
                taskCard(task).padding(.bottom, 11)
            }
        }
    }

    private func taskCard(_ task: ValveTaskStore.Task) -> some View {
        let valveName = [zh ? "双阀" : "Both", zh ? "阀 1" : "Valve 1", zh ? "阀 2" : "Valve 2"][task.valve]
        let dayList = zh ? dayNames : dayNamesEn
        return VStack(alignment: .leading, spacing: 6) {
            HStack {
                Text("#\(task.id)").font(.hkt(15, .bold)).foregroundStyle(Theme.text)
                StateBadge(kind: .info,
                           text: valveName + " · " + (task.state == 1 ? (zh ? "开阀" : "Open") : (zh ? "关阀" : "Close")))
                    .padding(.leading, 8)
                Spacer(minLength: 8)
                Button(zh ? "删除" : "Delete") {
                    deleteTask(task.id)
                }
                .font(.hkt(14, .semibold))
                .foregroundStyle(Theme.err)
            }
            Text("⏰ \(String(format: "%02d:%02d", task.sh, task.sm)) – \(String(format: "%02d:%02d", task.eh, task.em)) · "
                 + (zh ? "脉冲数" : "Pulse count") + " \(task.pulse)")
                .font(.hkt(13))
                .monospacedDigit()
                .foregroundStyle(Theme.text2)
            HStack(spacing: 4) {
                ForEach(0..<7, id: \.self) { index in
                    if task.days[index] {
                        Text(dayList[index])
                            .font(.hkt(12, .semibold))
                            .foregroundStyle(Theme.info)
                            .padding(.horizontal, 7).padding(.vertical, 2)
                            .background(Theme.info.opacity(0.15), in: Capsule())
                    }
                }
            }
        }
        .padding(EdgeInsets(top: 13, leading: 14, bottom: 13, trailing: 14))
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
            .stroke(Theme.line.opacity(0.82), lineWidth: 1))
        .hktShadow()
    }

    // MARK: - 编辑页（规格卡 §2）

    private var editPage: some View {
        VStack(spacing: 0) {
            NavbarHeader(title: zh ? "＋ 新建任务" : "＋ New Task",
                         backText: zh ? "‹ 返回" : "‹ Back",
                         onBack: { path = [] }) {
                EmptyView()
            }
            ScrollView {
                VStack(alignment: .leading, spacing: 0) {
                    if let validationMessage {
                        HKTBanner(kind: .err, text: "✕ " + validationMessage)
                    }
                    slotCard
                    choiceCard(label: zh ? "阀门" : "Valve",
                               options: [zh ? "双阀" : "Both", zh ? "阀 1" : "Valve 1", zh ? "阀 2" : "Valve 2"],
                               selection: $editDraft.valve)
                    choiceCard(label: zh ? "动作" : "Action",
                               options: [zh ? "开阀" : "Open", zh ? "关阀" : "Close"],
                               selection: Binding(get: { editDraft.state == 1 ? 0 : 1 },
                                                  set: { editDraft.state = $0 == 0 ? 1 : 0 }))
                    inputCard(label: zh ? "脉冲数" : "Pulse count", text: $editPulse)
                    timeCard
                    daysCard
                    Button {
                        saveTask()
                    } label: {
                        Text(zh ? "保存配置" : "Save")
                            .font(.hkt(16, .semibold))
                            .foregroundStyle(.white)
                            .frame(maxWidth: .infinity)
                            .padding(13)
                            .background(Theme.info, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
                    }
                    Button {
                        path = []
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
                    .padding(.top, 12)
                }
                .padding(.horizontal, 16)
                .padding(.bottom, 24)
            }
        }
    }

    /// 槽位格（1–16；选中=info、已占用=描边）。
    private var slotCard: some View {
        VStack(alignment: .leading, spacing: 0) {
            Text(zh ? "任务槽位（\(zh ? "1–16；写入覆盖该槽位现有任务" : "1–16; writing overwrites the task in that slot")）"
                    : "Task slot (1–16; writing overwrites the task in that slot)")
                .font(.hkt(12)).foregroundStyle(Theme.text2)
            LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 7), spacing: 5) {
                ForEach(1...16, id: \.self) { slot in
                    let used = store.isUsed(slot)
                    Button {
                        editDraft.id = slot
                    } label: {
                        Text("\(slot)")
                            .font(.hkt(13))
                            .foregroundStyle(editDraft.id == slot ? .white : Theme.text2)
                            .frame(maxWidth: .infinity)
                            .padding(.vertical, 8)
                            .background(editDraft.id == slot ? Theme.info : Theme.card2,
                                        in: RoundedRectangle(cornerRadius: 9))
                            .overlay(RoundedRectangle(cornerRadius: 9)
                                .stroke(used ? Theme.line : .clear, lineWidth: 1))
                    }
                }
            }
            .padding(.top, 8)
            Text(zh ? "实线圈 = 槽位已有任务，写入即覆盖" : "Outlined = slot in use; writing overwrites it")
                .font(.hkt(11)).foregroundStyle(Theme.text2)
                .padding(.top, 6)
        }
        .padding(EdgeInsets(top: 13, leading: 14, bottom: 13, trailing: 14))
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
            .stroke(Theme.line.opacity(0.82), lineWidth: 1))
        .hktShadow()
        .padding(.bottom, 9)
    }

    private func choiceCard(label: String, options: [String], selection: Binding<Int>) -> some View {
        VStack(alignment: .leading, spacing: 0) {
            Text(label).font(.hkt(12)).foregroundStyle(Theme.text2)
            HStack(spacing: 6) {
                ForEach(options.indices, id: \.self) { index in
                    let selected = selection.wrappedValue == index
                    Button {
                        selection.wrappedValue = index
                    } label: {
                        Text(options[index])
                            .font(.hkt(13, selected ? .semibold : .regular))
                            .foregroundStyle(selected ? .white : Theme.text2)
                            .frame(maxWidth: .infinity)
                            .padding(.vertical, 9)
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
        .padding(.bottom, 9)
    }

    private func inputCard(label: String, text: Binding<String>) -> some View {
        VStack(alignment: .leading, spacing: 0) {
            Text(label).font(.hkt(12)).foregroundStyle(Theme.text2)
            TextField("", text: text)
                .keyboardType(.numberPad)
                .font(.hkt(15, .semibold))
                .monospacedDigit()
                .foregroundStyle(Theme.text)
                .padding(.top, 6)
        }
        .padding(EdgeInsets(top: 13, leading: 14, bottom: 13, trailing: 14))
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
            .stroke(Theme.line.opacity(0.82), lineWidth: 1))
        .hktShadow()
        .padding(.bottom, 9)
    }

    /// 开始/结束时间（两组 时:分 菜单选择）。
    private var timeCard: some View {
        VStack(alignment: .leading, spacing: 0) {
            Text(zh ? "开始 / 结束" : "Start / End").font(.hkt(12)).foregroundStyle(Theme.text2)
            timePicker(row: "开始", hour: $editDraft.sh, minute: $editDraft.sm)
            Text("↓").font(.hkt(13)).foregroundStyle(Theme.text2)
                .frame(maxWidth: .infinity, alignment: .center)
                .padding(.vertical, 4)
            timePicker(row: "结束", hour: $editDraft.eh, minute: $editDraft.em)
        }
        .padding(EdgeInsets(top: 13, leading: 14, bottom: 13, trailing: 14))
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
            .stroke(Theme.line.opacity(0.82), lineWidth: 1))
        .hktShadow()
        .padding(.bottom, 9)
    }

    private func timePicker(row: String, hour: Binding<Int>, minute: Binding<Int>) -> some View {
        HStack(spacing: 6) {
            Text(row).font(.hkt(13)).foregroundStyle(Theme.text2)
                .frame(width: 30, alignment: .leading)
            Picker("", selection: hour) {
                ForEach(0..<24, id: \.self) { value in
                    Text(String(format: "%02d", value)).tag(value)
                }
            }
            .pickerStyle(.menu)
            .frame(maxWidth: .infinity)
            .background(Theme.card2, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
            .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius)
                .stroke(Theme.line, lineWidth: 1))
            Text(":").foregroundStyle(Theme.text2)
            Picker("", selection: minute) {
                ForEach(0..<60, id: \.self) { value in
                    Text(String(format: "%02d", value)).tag(value)
                }
            }
            .pickerStyle(.menu)
            .frame(maxWidth: .infinity)
            .background(Theme.card2, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
            .overlay(RoundedRectangle(cornerRadius: Theme.controlRadius)
                .stroke(Theme.line, lineWidth: 1))
        }
    }

    /// 重复日 chips（周一…周日 多选）。
    private var daysCard: some View {
        let dayList = zh ? dayNames : dayNamesEn
        return VStack(alignment: .leading, spacing: 0) {
            Text(zh ? "重复" : "Repeat").font(.hkt(12)).foregroundStyle(Theme.text2)
            HStack(spacing: 5) {
                ForEach(0..<7, id: \.self) { index in
                    Button {
                        editDraft.days[index].toggle()
                    } label: {
                        Text(dayList[index])
                            .font(.hkt(13))
                            .foregroundStyle(editDraft.days[index] ? .white : Theme.text2)
                            .padding(.vertical, 7).padding(.horizontal, 11)
                            .background(editDraft.days[index] ? Theme.info : Theme.card2,
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
        .padding(.bottom, 9)
    }

    // MARK: - 动作（真实 0x03/0x04/0x05：发送 → 设备 ACK 驱动结果；设备忙/拒绝=超时无 ACK）

    /// 立即执行（0x03）：校验 0–65535 → 发送；无 ACK（设备本地任务执行中被静默忽略）显示 busy 横幅。
    /// 阀行点按（0x03）：校验 → 发送 → ACK 驱动状态机。
    /// 纯开关（dur=0&pulse=0）→ done 闪条 2s；定时/脉冲 → 执行中（dur>0 本地倒计时镜像）；
    /// 无 ACK（仅定时任务执行中才可能）→ busy 横幅 2.6s。
    private func sendExec(_ valve: Int, _ state: Int) {
        guard rt?.phase != .sending else { return }
        let dur = Int(rtDur) ?? 0
        let pulse = Int(rtPulse) ?? 0
        guard (0...65535).contains(dur), (0...65535).contains(pulse) else { return }
        busyVisible = false
        let cmd = RTExec(phase: .sending, valve: valve, state: state,
                         dur: dur, pulse: pulse, remain: dur, flash: nil)
        rt = cmd
        LogStore.shared.info("0x03 " + (zh ? "执行 valve=\(valve) state=\(state) dur=\(dur) pulse=\(pulse)"
                                           : "exec valve=\(valve) state=\(state) dur=\(dur) pulse=\(pulse)"))
        Task {
            let acked = await session.sendWrite(
                cmd: CommandCode.svcRealtimeTask,
                data: HKTFrameEncoder.svcRealtimeTaskPayload(valve: valve, state: state,
                                                             durationS: dur, pulse: pulse))
            guard rt == cmd else { return }   // 已被新命令取代或页面退出清理
            if acked {
                LogStore.shared.info("0x03 ACK（设备已开始执行）")
                if dur > 0 || pulse > 0 {
                    rt = RTExec(phase: .executing, valve: valve, state: state,
                                dur: dur, pulse: pulse, remain: dur, flash: nil)
                    if dur > 0 { startCountdown(cmd) }
                    // dur=0 pulse>0：执行中持续（设备脉冲打满自停），直至用户下发新命令
                } else {
                    // 纯开关：保持该状态直至下一命令（固件语义），直达完成闪条
                    var done = RTExec(phase: .done, valve: valve, state: state,
                                      dur: dur, pulse: pulse, remain: 0, flash: nil)
                    done.flash = "✓ \(valveName(valve)) " + (state == 1 ? (zh ? "开阀" : "open") : (zh ? "关阀" : "close"))
                        + " · " + (zh ? "已执行 · 阀门已动作" : "executed · valve actuated")
                    LogStore.shared.info("0x03 " + (zh ? "阀门已动作（保持直至下一命令）" : "valve actuated (hold until next command)"))
                    rt = done
                    try? await Task.sleep(for: .seconds(2))
                    if rt == done { rt = nil }
                }
            } else {
                rt = nil
                busyVisible = true
                LogStore.shared.warn("0x03 " + (zh ? "无 ACK（设备忙或已拒绝）" : "no ACK (busy or rejected)"))
                try? await Task.sleep(for: .seconds(2.6))
                busyVisible = false
            }
        }
    }

    /// 定时回位倒计时（本地镜像：固件无完成上报，结束时设备自动反向复位阀门）。
    private func startCountdown(_ cmd: RTExec) {
        countdownTask?.cancel()
        countdownTask = Task {
            while rt == cmd, rt?.phase == .executing, !Task.isCancelled {
                try? await Task.sleep(for: .seconds(1))
                guard rt == cmd, rt?.phase == .executing else { return }
                rt?.remain -= 1
                if rt?.remain ?? 0 <= 0 {
                    rt?.phase = .done
                    rt?.flash = nil
                    LogStore.shared.info("0x03 " + (zh ? "定时回位结束（本地镜像）：设备已自动复位阀门"
                                                       : "timed hold done (local mirror): valve restored by device"))
                    try? await Task.sleep(for: .seconds(3))
                    if rt?.phase == .done { rt = nil }
                    return
                }
            }
        }
    }

    /// 删除任务（0x05）：确认后更新镜像；删除执行中任务设备会强制停止阀门动作。
    private func deleteTask(_ id: Int) {
        Task {
            let acked = await session.sendWrite(cmd: CommandCode.svcDeleteTask,
                                                data: HKTFrameEncoder.svcDeleteTaskPayload(id: id))
            guard acked else {
                LogStore.shared.info("0x05 " + (zh ? "无 ACK，未删除 #\(id)" : "no ACK, #\(id) not deleted"))
                return
            }
            store.delete(id)
            LogStore.shared.info("0x05 " + (zh ? "删除任务" : "delete task") + " #\(id) ACK")
        }
    }

    /// 全部删除（0x05 id=0xFF）。
    private func deleteAll() {
        Task {
            let acked = await session.sendWrite(cmd: CommandCode.svcDeleteTask,
                                                data: HKTFrameEncoder.svcDeleteTaskPayload(id: 0xFF))
            guard acked else {
                LogStore.shared.info("0x05 " + (zh ? "全部删除无 ACK" : "delete ALL no ACK"))
                return
            }
            store.deleteAll()
            LogStore.shared.info("0x05 " + (zh ? "全部删除 (0xFF) ACK" : "delete ALL (0xFF) ACK"))
        }
    }

    /// 新建任务：取未占用最小槽位。
    private func newTask() {
        validationMessage = nil
        var used = Set(store.tasks.map(\.id))
        var slot = 1
        while used.contains(slot), slot < 16 { slot += 1 }
        used.insert(slot)
        editDraft = ValveTaskStore.Task(
            id: slot, valve: 1, state: 1, pulse: 100,
            sh: 8, sm: 0, eh: 18, em: 30,
            days: [true, true, true, true, true, false, false])
        editPulse = "100"
    }

    /// 保存任务（0x04）：前置校验 → 发送；ACK 后才写入镜像（固件对非法参数静默拒绝）。
    private func saveTask() {
        let pulse = Int(editPulse)
        if editDraft.eh * 60 + editDraft.em <= editDraft.sh * 60 + editDraft.sm {
            validationMessage = zh ? "时间无效（结束须晚于开始）" : "Invalid time (end must be after start)"
            return
        }
        guard let pulse, (0...65535).contains(pulse) else {
            validationMessage = zh ? "超出范围 0–65535" : "Out of range 0–65535"
            return
        }
        guard editDraft.days.contains(true) else {
            validationMessage = zh ? "至少选择一天" : "Pick at least one day"
            return
        }
        guard !savePending else { return }
        validationMessage = nil
        savePending = true
        // 重复位：days[0]=周一 … days[6]=周日 → bit0…bit6（固件 repeat_duty，0x7F 上限）
        let repeatMask = editDraft.days.enumerated().reduce(0) { mask, pair in
            pair.element ? mask | (1 << pair.offset) : mask
        }
        LogStore.shared.info("0x04 " + (zh ? "任务" : "task") + " #\(editDraft.id)")
        Task {
            let acked = await session.sendWrite(
                cmd: CommandCode.svcTimedTask,
                data: HKTFrameEncoder.svcTimedTaskPayload(
                    id: editDraft.id, valve: editDraft.valve, state: editDraft.state,
                    pulse: pulse, startMinute: editDraft.sh * 60 + editDraft.sm,
                    endMinute: editDraft.eh * 60 + editDraft.em, repeatMask: repeatMask))
            savePending = false
            if acked {
                var saved = editDraft
                saved.pulse = pulse   // 输入框绑定的是 editPulse 字符串，镜像须回写真实输入值
                store.upsert(saved)
                LogStore.shared.info("0x04 " + (zh ? "任务" : "task") + " #\(editDraft.id) ACK")
                path = []
            } else {
                validationMessage = zh ? "设备未确认（可能被固件拒绝），请检查参数后重试"
                                       : "Device did not acknowledge (possibly rejected); check values and retry"
            }
        }
    }
}
