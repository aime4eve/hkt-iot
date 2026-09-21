"""假桥+假设备：验证 harness 管道（无蓝牙）。模拟 MacBLEBridge 的 TCP 协议与 HKT 设备应答。"""
import socket, threading, json, time

ACK = bytes.fromhex("686B740000FF00")
FIXTURES = {
    "MPS100": bytes.fromhex("686B740001010B1C8D0103573A013B0084008600145D00785EFF885F019060000A0014001E00280032003C00460050005A0064"),
    "UDS100": bytes.fromhex("686B74000201020A8D018B0FA0090061A80A002EE00E00144400280045003C46043847004801900BB886001E10010203041105060708"),
    "SVC100": bytes.fromhex("686B740003010D0D8D0103633C01000064010101F440024101420543018A1986001E"),
}
DEVICES = [("MPS100_9C01", "DEV-MPS-UUID", -61), ("UDS100_3F2A", "DEV-UDS-UUID", -66)]

class Client(threading.Thread):
    def __init__(self, conn):
        super().__init__(daemon=True)
        self.conn = conn
        self.family = "MPS100"
        self.buf = b""
    def send(self, obj):
        self.conn.sendall((json.dumps(obj) + "\n").encode())
    def run(self):
        while True:
            data = self.conn.recv(65536)
            if not data:
                return
            self.buf += data
            while b"\n" in self.buf:
                line, self.buf = self.buf.split(b"\n", 1)
                if line.strip():
                    self.handle(json.loads(line))
    def handle(self, obj):
        cmd = obj.get("cmd")
        if cmd == "ping":
            self.send({"ev": "pong"})
        elif cmd == "state":
            self.send({"ev": "state", "state": 5, "auth": 2})
        elif cmd == "scan":
            for name, ident, rssi in DEVICES:
                self.send({"ev": "scan", "name": name, "id": ident, "rssi": rssi})
        elif cmd == "connect":
            for name, ident, _ in DEVICES:
                if ident == obj.get("id"):
                    self.family = name[:6]
            threading.Timer(0.3, lambda: self.send({"ev": "ready"})).start()
        elif cmd == "write":
            frame = bytes.fromhex(obj["hex"])
            resp = self.respond(frame)
            if resp:
                time.sleep(0.03)
                self.send({"ev": "rx", "hex": resp.hex().upper()})
    def respond(self, frame):
        if len(frame) > 14 and frame[-8:] == b"bootload":
            body_len = (frame[3] << 8) | frame[4]
            bcmd = frame[5]
            if bcmd == 1 and body_len == 5:
                return bytes([0x68, 0x6B, 0x74, 0x02, 0x00, 0x00]) + b"bootload"  # ACK(2,0) 14B
            if bcmd == 2:
                n = (frame[6] << 8) | frame[7]
                nxt = n + 1
                return bytes([0x68, 0x6B, 0x74, 0x02, (nxt >> 8) & 0xFF, nxt & 0xFF]) + b"bootload"
            if bcmd == 0xFF:
                return bytes([0x68, 0x6B, 0x74, 0x03, 0x00, 0x00]) + b"bootload"
            return None
        if len(frame) > 6:
            c = frame[6]
            if c == 0xFF:
                return FIXTURES[self.family]
            if c in (0x02, 0x03, 0x04, 0x05, 0xFD, 0xFE):
                return ACK
        return None

srv = socket.socket()
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("127.0.0.1", 9876))
srv.listen(2)
print("fake bridge on 9876", flush=True)
while True:
    conn, _ = srv.accept()
    Client(conn).start()
