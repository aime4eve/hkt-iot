import CoreBLE
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
/// 真实 0x03/0x04/0x05 发送随协议里程碑接入；当前本地镜像+日志与原型演示一致。
struct TasksView: View {
    @Environment(LanguageStore.self) private var langStore
    @Environment(\.dismiss) private var dismiss
    @State private var path: [Int] = []          // [0] = 编辑页

    @State private var rtValve = 0               // 0 双阀 / 1 阀1 / 2 阀2
    @State private var rtState = 1               // 1 开 / 0 关
    @State private var rtDur = "60"
    @State private var rtPulse = "100"
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

    private let dayNames = ["周一", "周二", "周三", "周四", "周五", "周六", "周日"]
    private let dayNamesEn = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]

    var body: some View {
        VStack(spacing: 0) {
            if path.isEmpty { listPage } else { editPage }
        }
        .background(Theme.bg)
        .toolbar(.hidden, for: .navigationBar)
        .onAppear {
            // 演示自动导航（-demo-page task-edit）：直接进入编辑页
            if ProcessInfo.processInfo.arguments.contains("-demo-page task-edit") {
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
                            store.deleteAll()
                            LogStore.shared.info("0x05 " + (zh ? "全部删除 (0xFF)" : "delete ALL (0xFF)"))
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

    /// 实时任务卡（0x03）。
    private var rtCard: some View {
        VStack(alignment: .leading, spacing: 0) {
            Text((zh ? "实时任务" : "Realtime Task") + " · 0x03")
                .font(.hkt(15, .bold))
                .foregroundStyle(Theme.text)
                .padding(.bottom, 8)
            HStack(spacing: 8) {
                chipField(label: zh ? "阀门" : "Valve",
                          options: [zh ? "双阀" : "Both", zh ? "阀 1" : "Valve 1", zh ? "阀 2" : "Valve 2"],
                          selection: $rtValve)
                chipField(label: zh ? "动作" : "Action",
                          options: [zh ? "开阀" : "Open", zh ? "关阀" : "Close"],
                          selection: Binding(get: { rtState == 1 ? 0 : 1 },
                                             set: { rtState = $0 == 0 ? 1 : 0 }))
            }
            HStack(spacing: 9) {
                fieldCard(label: (zh ? "持续时间" : "Duration") + "(s)") {
                    TextField("", text: $rtDur).keyboardType(.numberPad)
                    Text(zh ? "0 = 不自动结束（须手动停止）" : "0 = no auto-stop (stop manually)")
                        .font(.hkt(11)).foregroundStyle(Theme.text2)
                }
                fieldCard(label: zh ? "脉冲数" : "Pulse count") {
                    TextField("", text: $rtPulse).keyboardType(.numberPad)
                }
            }
            .padding(.top, 9)
            if busyVisible {
                HKTBanner(kind: .err,
                          text: "⏱ " + (zh ? "设备忙（本地定时任务执行中），实时任务被静默忽略（无 ACK）"
                                            : "Device busy (schedule running); realtime task silently ignored (no ACK)"))
                    .padding(.top, 9)
            }
            Button {
                execRT()
            } label: {
                Text(zh ? "立即执行" : "Run Now")
                    .font(.hkt(16, .semibold))
                    .foregroundStyle(.white)
                    .frame(maxWidth: .infinity)
                    .padding(13)
                    .background(Theme.info, in: RoundedRectangle(cornerRadius: Theme.controlRadius))
            }
            .padding(.top, 12)
        }
        .padding(EdgeInsets(top: 13, leading: 14, bottom: 13, trailing: 14))
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(Theme.card, in: RoundedRectangle(cornerRadius: Theme.cardRadius))
        .overlay(RoundedRectangle(cornerRadius: Theme.cardRadius)
            .stroke(Theme.line.opacity(0.82), lineWidth: 1))
        .hktShadow()
        .padding(.bottom, 9)
    }

    /// 芯片选择组（实时任务卡内 .field 样式）。
    private func chipField(label: String, options: [String], selection: Binding<Int>) -> some View {
        VStack(alignment: .leading, spacing: 0) {
            Text(label).font(.hkt(11, .semibold)).foregroundStyle(Theme.text2)
                .lineLimit(1)
            Text(options[selection.wrappedValue])
                .font(.hkt(14, .semibold))
                .foregroundStyle(Theme.text)
                .padding(.top, 4)
            HStack(spacing: 4) {
                ForEach(options.indices, id: \.self) { index in
                    Button {
                        selection.wrappedValue = index
                    } label: {
                        Text(options[index])
                            .font(.hkt(12))
                            .foregroundStyle(selection.wrappedValue == index ? .white : Theme.text2)
                            .frame(maxWidth: .infinity)
                            .padding(.vertical, 6)
                            .background(selection.wrappedValue == index ? Theme.info : Theme.card2,
                                        in: RoundedRectangle(cornerRadius: 8))
                    }
                }
            }
            .padding(.top, 6)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
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
                    store.delete(task.id)
                    LogStore.shared.info("0x05 " + (zh ? "删除任务" : "delete task") + " #\(task.id)")
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

    // MARK: - 动作（execRT / saveTask / newTask；真实命令随协议里程碑接入）

    /// 立即执行（0x03）：校验 0–65535；busy 演示 2.6s；成功更新本地演示状态。
    private func execRT() {
        let duration = Int(rtDur)
        let pulse = Int(rtPulse)
        guard let duration, let pulse, (0...65535).contains(duration), (0...65535).contains(pulse) else {
            busyVisible = false
            return
        }
        LogStore.shared.info("0x03 " + (zh ? "执行 valve=\(rtValve) state=\(rtState)"
                                           : "exec valve=\(rtValve) state=\(rtState)"))
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

    /// 保存任务（0x04）：前置校验 → 镜像 upsert + 日志。
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
        validationMessage = nil
        store.upsert(editDraft)
        LogStore.shared.info("0x04 " + (zh ? "任务" : "task") + " #\(editDraft.id) ACK")
        path = []
    }
}
