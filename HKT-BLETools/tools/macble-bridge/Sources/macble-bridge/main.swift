// macOS BLE 桥（M7 真机验证用）：CoreBluetooth ↔ TCP 行 JSON，供 JVM 侧
// Android v2 协议核心（core-ble DeviceSession / core-ota OtaEngine）驱动真机。
// 协议（行分隔 JSON）：
//   → {"cmd":"ping"}                      ← {"ev":"pong"}
//   → {"cmd":"scan"} / {"cmd":"scanStop"} ← {"ev":"scan","name":..,"id":..,"rssi":-NN}
//   → {"cmd":"connect","id":"UUID"}       ← {"ev":"ready"} | {"ev":"connectFailed","reason":..}
//   → {"cmd":"disconnect"}                ← {"ev":"disc"}（也用于对端断开通知）
//   → {"cmd":"write","hex":"686B.."}      ←（数据经 {"ev":"rx","hex":..} 回流）
import CoreBluetooth
import Foundation
import Network

let serviceUUID = CBUUID(string: "0783B03E-8535-B5A0-7140-A304D2495CB7")
let indicateUUID = CBUUID(string: "0783B03E-8535-B5A0-7140-A304D2495CB8")
let writeUUID = CBUUID(string: "0783B03E-8535-B5A0-7140-A304D2495CBA")

let namePrefixes = ["MPS", "SVC", "UDS", "EPS", "DC"]

final class Bridge: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    var central: CBCentralManager!
    var connection: NWConnection?
    var discovered: [UUID: CBPeripheral] = [:]
    var peripheral: CBPeripheral?
    var writeChar: CBCharacteristic?
    var scanWanted = false

    func send(_ dict: [String: Any]) {
        guard let conn = connection, conn.state == .ready else { return }
        guard var data = try? JSONSerialization.data(withJSONObject: dict) else { return }
        data.append(0x0A)
        conn.send(content: data, completion: .contentProcessed { _ in })
    }

    // MARK: - TCP

    func startTCP() {
        let listener = try! NWListener(using: .tcp, on: NWEndpoint.Port(9876))
        listener.newConnectionHandler = { [weak self] (conn: NWConnection) in
            guard let self = self, self.connection == nil || self.connection?.state != .ready else {
                conn.cancel()
                return
            }
            self.connection = conn
            conn.stateUpdateHandler = { (state: NWConnection.State) in
                switch state {
                case .failed, .cancelled:
                    self.connection = nil
                    // TCP 断开联动断 BLE：设备干净回广播态，下一轮是全新连接
                    if let p = self.peripheral {
                        self.central.cancelPeripheralConnection(p)
                    }
                default: break
                }
            }
            conn.start(queue: .main)
            self.pump(conn)
        }
        listener.start(queue: DispatchQueue.main)
        print("bridge: tcp 127.0.0.1:9876")
    }

    func pump(_ conn: NWConnection) {
        conn.receive(minimumIncompleteLength: 1, maximumLength: 65536) { [weak self] content, _, _, error in
            guard let self = self else { return }
            if error != nil { conn.cancel(); return }
            if let content = content, !content.isEmpty {
                self.ingest(content)
            }
            self.pump(conn)
        }
    }

    var rxBuffer = Data()
    func ingest(_ content: Data) {
        rxBuffer.append(content)
        while let nl = rxBuffer.firstIndex(of: 0x0A) {
            let lineData = rxBuffer.subdata(in: rxBuffer.startIndex..<nl)
            rxBuffer.removeSubrange(rxBuffer.startIndex...nl)
            if let line = String(data: lineData, encoding: .utf8), !line.isEmpty {
                handle(line)
            }
        }
    }

    func handle(_ line: String) {
        guard let d = try? JSONSerialization.jsonObject(with: Data(line.utf8)) as? [String: Any],
              let cmd = d["cmd"] as? String else { return }
        switch cmd {
        case "ping":
            send(["ev": "pong"])
        case "state":
            send(["ev": "state", "state": bridge.central?.state.rawValue ?? -1,
                  "auth": bridge.central?.authorization.rawValue ?? -1])
        case "scan":
            scanWanted = true
            discovered.removeAll()
            central.scanForPeripherals(withServices: nil)
        case "scanStop":
            scanWanted = false
            central.stopScan()
        case "connect":
            guard let idStr = d["id"] as? String, let id = UUID(uuidString: idStr),
                  let p = discovered[id] else {
                send(["ev": "connectFailed", "reason": "unknown device id"])
                return
            }
            peripheral = p
            p.delegate = self
            central.connect(p, options: nil)
        case "disconnect":
            if let p = peripheral { central.cancelPeripheralConnection(p) }
        case "write":
            guard let hex = d["hex"] as? String, let bytes = hexToBytes(hex), let c = writeChar else {
                send(["ev": "writeFailed"])
                return
            }
            let supportsWithResponse = c.properties.contains(.write)
            peripheral?.writeValue(Data(bytes), for: c, type: supportsWithResponse ? .withResponse : .withoutResponse)
        default:
            break
        }
    }

    // MARK: - CBCentralManagerDelegate

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        print("bridge: central state=\(central.state.rawValue)")
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                        advertisementData: [String: Any], rssi RSSI: NSNumber) {
        let name = (advertisementData[CBAdvertisementDataLocalNameKey] as? String)
            ?? peripheral.name ?? ""
        guard namePrefixes.contains(where: { name.hasPrefix($0) }) else { return }
        discovered[peripheral.identifier] = peripheral
        send(["ev": "scan", "name": name, "id": peripheral.identifier.uuidString, "rssi": RSSI.intValue])
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        peripheral.delegate = self
        peripheral.discoverServices([serviceUUID])
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral,
                        error: Error?) {
        send(["ev": "connectFailed", "reason": error.map(String.init(describing:)) ?? "unknown"])
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral,
                        error: Error?) {
        writeChar = nil
        send(["ev": "disc"])
    }

    // MARK: - CBPeripheralDelegate

    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let service = peripheral.services?.first(where: { $0.uuid == serviceUUID }) else {
            send(["ev": "connectFailed", "reason": "HKT service not found"])
            return
        }
        peripheral.discoverCharacteristics([indicateUUID, writeUUID], for: service)
    }

    func peripheral(_ peripheral: CBPeripheral,
                    didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard error == nil,
              let indicate = service.characteristics?.first(where: { $0.uuid == indicateUUID }),
              let write = service.characteristics?.first(where: { $0.uuid == writeUUID }) else {
            send(["ev": "connectFailed", "reason": "HKT characteristics not found"])
            return
        }
        writeChar = write
        FileHandle.standardError.write(Data("[diag] write char properties=\(write.properties.rawValue) indicate=\(indicate.properties.rawValue)\n".utf8))
        if indicate.isNotifying {
            send(["ev": "ready"])   // 幂等重连：重复 setNotify 不再触发状态回调
        } else {
            peripheral.setNotifyValue(true, for: indicate)
        }
    }

    func peripheral(_ peripheral: CBPeripheral,
                    didUpdateNotificationStateFor characteristic: CBCharacteristic, error: Error?) {
        if characteristic.uuid == indicateUUID {
            if error == nil {
                send(["ev": "ready"])
            } else {
                send(["ev": "connectFailed", "reason": "subscribe failed"])
            }
        }
    }

    func peripheral(_ peripheral: CBPeripheral,
                    didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        if let e = error {
            FileHandle.standardError.write(Data("[diag] value error: \(e)\n".utf8))
        }
        guard characteristic.uuid == indicateUUID, let v = characteristic.value else { return }
        send(["ev": "rx", "hex": v.map { String(format: "%02X", $0) }.joined()])
    }

    func peripheral(_ peripheral: CBPeripheral,
                    didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
        if let e = error {
            FileHandle.standardError.write(Data("[diag] write error: \(e)\n".utf8))
        } else {
            FileHandle.standardError.write(Data("[diag] write acked by device\n".utf8))
        }
    }
}

func hexToBytes(_ s: String) -> [UInt8]? {
    guard s.count % 2 == 0 else { return nil }
    var bytes: [UInt8] = []
    var index = s.startIndex
    while index < s.endIndex {
        let next = s.index(index, offsetBy: 2)
        guard let b = UInt8(s[index..<next], radix: 16) else { return nil }
        bytes.append(b)
        index = next
    }
    return bytes
}


setvbuf(stdout, nil, _IOLBF, 0)
let bridge = Bridge()
bridge.startTCP()
FileHandle.standardError.write(Data("[bridge] started\n".utf8))
// CBCentralManager 回调依赖主 RunLoop 泵（dispatchMain 不泵 RunLoop，XPC 代理不回调——实测踩坑）
DispatchQueue.main.async {
    bridge.central = CBCentralManager(delegate: bridge, queue: .main)
    DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) {
        var info = "state=\(bridge.central.state.rawValue)"
        if #available(macOS 13.0, *) {
            info += " authorization=\(bridge.central.authorization.rawValue)"
        }
        FileHandle.standardError.write(Data("[probe] \(info)\n".utf8))
    }
}
RunLoop.main.run()
