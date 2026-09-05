#!/usr/bin/env python3
"""Generate shared protocol golden vectors (M3 baseline).

Authority: firmware source (see shared/devices/firmware-traceability.md).
- CRC16-CCITT reflected (KERMIT): poly 0x8408, init 0x0000, xor-out 0x0000.
- TX app frame: hkt(3) packNum(1) len(2 BE) cmd(1) data(n) crc(2 over cmd+data)
- Device response: hkt(3) 0x00 seq(1) fixedSizeTLV stream (no len/cmd/crc)
- Bootloader frame: hkt(3) len(2 BE) cmd(1) packNum(2 BE) data(n) crc(2) "bootload"(8)

Run from inside shared/fixtures/:  python3 gen_fixtures.py (writes next to itself)
"""
import json
import os
from pathlib import Path

BOOTLOAD = bytes.fromhex("626F6F746C6F6164")


def crc16(data: bytes) -> int:
    crc = 0x0000
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0x8408 if crc & 1 else crc >> 1
    return crc


def h(payload) -> str:
    return bytes(payload).hex().upper()


def tx_frame(pack_num: int, cmd: int, data: bytes) -> str:
    body = bytes([cmd]) + data
    frame = bytes([0x68, 0x6B, 0x74, pack_num]) + len(body).to_bytes(2, "big") + body
    crc = crc16(body)
    return (frame + crc.to_bytes(2, "big")).hex().upper()


def bt_frame(cmd: int, pack_num: int, data: bytes) -> str:
    body = bytes([cmd]) + pack_num.to_bytes(2, "big") + data
    frame = bytes([0x68, 0x6B, 0x74]) + len(body).to_bytes(2, "big") + body
    crc = crc16(body)
    return (frame + crc.to_bytes(2, "big") + BOOTLOAD).hex().upper()


def dev_response(seq: int, tlvs) -> str:
    stream = b"".join(bytes([t]) + v for t, v in tlvs)
    return (bytes([0x68, 0x6B, 0x74, 0x00, seq]) + stream).hex().upper()


def main():
    if not os.path.basename(os.getcwd()) == "fixtures":
        raise SystemExit("run from inside shared/fixtures/: python3 gen_fixtures.py")

    FILL4 = bytes([0xFF] * 4)

    tx = [
        {
            "id": "TX-QUERY-001", "device": "ALL", "cmd": "0xFF",
            "purpose": "status query (Android sends 4-byte FF filler)",
            "request": tx_frame(0x00, 0xFF, FILL4), "expectedAck": None,
            "firmwareReference": "*/USER/Drive/communicate.c data[6]==0xFF branch",
            "androidReference": "Communicate.kt streamDevice(0xFF)",
            "iosTest": "FrameCodecTests/testQueryFrame",
        },
        {
            "id": "TX-DC200-CFG-001", "device": "DC200Family", "cmd": "0x02",
            "purpose": "report period 30min BE + park mode 1 (accepted)",
            "request": tx_frame(0x00, 0x02, bytes([0x00, 0x1E, 0x01])),
            "expectedAck": dev_response(0, [(0xFF, b"\xFF")]),
            "firmwareReference": "ParkingSensor communicate.c data[5]==4&&data[6]==2 branch",
            "androidReference": "Communicate.kt streamDevice(0x02, DC200Family layout)",
            "iosTest": "FrameCodecTests/testDC200ConfigFrame",
        },
        {
            "id": "TX-DC200-CFG-002", "device": "DC200Family", "cmd": "0x02",
            "purpose": "period 0 accepted by firmware (boundary); mode must stay 0-2",
            "request": tx_frame(0x00, 0x02, bytes([0x00, 0x00, 0x01])),
            "expectedAck": dev_response(0, [(0xFF, b"\xFF")]),
            "firmwareReference": "ParkingSensor communicate.c: reportInterval==0 accepted",
            "iosTest": "ConfigBoundaryTests/testDC200ZeroPeriod",
        },
        {
            "id": "TX-DC200-CFG-003", "device": "DC200Family", "cmd": "0x02",
            "purpose": "park mode 3 out of range -> no ACK (silent)",
            "request": tx_frame(0x00, 0x02, bytes([0x00, 0x1E, 0x03])),
            "expectedAck": None,
            "firmwareReference": "ParkingSensor communicate.c: parkMode>2 ERROR tag only",
            "iosTest": "ConfigBoundaryTests/testDC200ModeRejected",
        },
        {
            "id": "TX-UDS-CFG-001", "device": "UDS100", "cmd": "0x02",
            "purpose": "report 30 + gps 60 + low 300 + high 3000 (all BE, accepted)",
            "request": tx_frame(0x00, 0x02, bytes([0x00, 0x1E, 0x00, 0x3C, 0x01, 0x2C, 0x0B, 0xB8])),
            "expectedAck": dev_response(0, [(0xFF, b"\xFF")]),
            "firmwareReference": "UDS communicate.c data[5]==9&&data[6]==2 branch",
            "iosTest": "FrameCodecTests/testUDS100ConfigFrame",
        },
        {
            "id": "TX-UDS-CFG-002", "device": "UDS100", "cmd": "0x02",
            "purpose": "low threshold 29 (<30) -> whole config silently rejected, no ACK",
            "request": tx_frame(0x00, 0x02, bytes([0x00, 0x1E, 0x00, 0x3C, 0x00, 0x1D, 0x0B, 0xB8])),
            "expectedAck": None,
            "firmwareReference": "UDS communicate.c: low<30 return (no callback_BLEAck)",
            "iosTest": "ConfigBoundaryTests/testUDS100LowThresholdRejected",
        },
        {
            "id": "TX-SVC-CFG-001", "device": "SVC100", "cmd": "0x02",
            "purpose": "vol 2 + mode 1 + stable 5 + autoPower 1 + tz 25(+03:30) + period 30",
            "request": tx_frame(0x00, 0x02, bytes([0x02, 0x01, 0x05, 0x01, 0x19, 0x00, 0x1E])),
            "expectedAck": dev_response(0, [(0xFF, b"\xFF")]),
            "firmwareReference": "SVC communicate.c data[5]==8&&data[6]==2 branch",
            "iosTest": "FrameCodecTests/testSVC100ConfigFrame",
        },
        {
            "id": "TX-SVC-CFG-002", "device": "SVC100", "cmd": "0x02",
            "purpose": "timezone 27 (>26) -> early return, no ACK even if period valid",
            "request": tx_frame(0x00, 0x02, bytes([0x02, 0x01, 0x05, 0x01, 0x1B, 0x00, 0x1E])),
            "expectedAck": None,
            "firmwareReference": "SVC communicate.c: timezone>26 return before ack",
            "iosTest": "ConfigBoundaryTests/testSVC100TimezoneRejected",
        },
        {
            "id": "TX-SVC-TASK-001", "device": "SVC100", "cmd": "0x03",
            "purpose": "realtime task: valve 1 + state 1 + duration 5 s (2B BE, 0=no auto-stop) + pulse 100 (3B BE); "
                       "silently ignored (no ACK) while a local schedule task is running",
            "request": tx_frame(0x00, 0x03, bytes([0x01, 0x01, 0x00, 0x05, 0x00, 0x64])),
            "expectedAck": dev_response(0, [(0xFF, b"\xFF")]),
            "firmwareReference": "SVC communicate.c data[5]==7&&data[6]==3 branch; busy -> branch skipped silently",
            "iosTest": "FrameCodecTests/testSVC100RealtimeTask",
        },
        {
            "id": "TX-SVC-TASK-002", "device": "SVC100", "cmd": "0x04",
            "purpose": "timed task: id 1 + valve 1 + state 1 + pulse 100 + 08:00-18:30 (minute-of-day BE) + repeat 0x7F (bit0=Mon..bit6=Sun)",
            "request": tx_frame(0x00, 0x04, bytes([0x01, 0x01, 0x01, 0x00, 0x64, 0x01, 0xE0, 0x04, 0x56, 0x7F])),
            "expectedAck": dev_response(0, [(0xFF, b"\xFF")]),
            "firmwareReference": "SVC communicate.c data[5]==11&&data[6]==4 branch; invalid id/valve/state/time/repeat -> silent return",
            "iosTest": "FrameCodecTests/testSVC100TimedTask",
        },
        {
            "id": "TX-SVC-TASK-003", "device": "SVC100", "cmd": "0x05",
            "purpose": "delete all tasks (0xFF); single delete = id 1-16; deleting a running task force-stops it; ACK always",
            "request": tx_frame(0x00, 0x05, bytes([0xFF])),
            "expectedAck": dev_response(0, [(0xFF, b"\xFF")]),
            "firmwareReference": "SVC communicate.c data[5]==2&&data[6]==5 branch",
            "iosTest": "FrameCodecTests/testSVC100DeleteAllTasks",
        },
        {
            "id": "TX-POWER-001", "device": "ALL", "cmd": "0xFE",
            "purpose": "power on; power byte is payload[3] (frame offset 10, firmware reads data[10])",
            "request": tx_frame(0x00, 0xFE, bytes([0x00, 0x00, 0x00, 0x01])),
            "expectedAck": dev_response(0, [(0xFF, b"\xFF")]),
            "firmwareReference": "*/communicate.c data[6]==0xFE reads data[10]",
            "androidReference": "Communicate.kt streamDevice(0xFE) %08X filler",
            "iosTest": "FrameCodecTests/testPowerFrame",
        },
        {
            "id": "TX-CAL-001", "device": "ALL", "cmd": "0xFD",
            "purpose": "calibration trigger, filler payload, ACK immediate (completion async)",
            "request": tx_frame(0x00, 0xFD, FILL4),
            "expectedAck": dev_response(0, [(0xFF, b"\xFF")]),
            "firmwareReference": "*/communicate.c data[6]==0xFD",
            "iosTest": "FrameCodecTests/testCalibrationFrame",
        },
        {
            "id": "TX-SYNC-001", "device": "ALL", "cmd": "0x06",
            "purpose": "time sync 4B BE unix stamp, firmware adds fixed UTC+8 (SVC uses tz)",
            "request": tx_frame(0x00, 0x06, (1767139200).to_bytes(4, "big")),
            "expectedAck": dev_response(0, [(0xFF, b"\xFF")]),
            "firmwareReference": "*/communicate.c data[5]==4&&data[6]==6 branch",
            "iosTest": "FrameCodecTests/testTimeSyncFrame",
        },
    ]

    resp = [
        {
            "id": "RX-DC200-QUERY-001", "device": "DC200Family",
            "purpose": "query response: ver,power,battery,park,mode,tamper,period,magXYZ,radar",
            "byteOrder": "bigEndian",
            "response": dev_response(1, [
                (0x01, bytes([0x0B, 0x1C])), (0x8D, b"\x01"), (0x03, b"\x57"),
                (0x3A, b"\x01"), (0x3B, b"\x00"), (0x84, b"\x00"),
                (0x86, bytes([0x00, 0x14])),
                (0x5D, bytes([0x00, 0x78])), (0x5E, bytes([0xFF, 0x88])), (0x5F, bytes([0x01, 0x90])),
                (0x60, bytes([0, 10, 0, 20, 0, 30, 0, 40, 0, 50, 0, 60, 0, 70, 0, 80, 0, 90, 0, 100])),
            ]),
            "expected": {
                "hardwareVersion": "0x0B", "softwareVersion": "0x1C", "power": 1, "battery": 87,
                "parkState": 1, "parkMode": 0, "tamper": 0, "reportPeriod": 20,
                "magX": 120, "magY": -120, "magZ": 400,
                "radarSpectrum": [10, 20, 30, 40, 50, 60, 70, 80, 90, 100],
            },
            "firmwareReference": "ParkingSensor communicate.c callback_BLEQuery + setDataPackage sizes",
            "iosTest": "TLVParserTests/testDC200FamilyQueryResponse",
        },
        {
            "id": "RX-UDS-QUERY-001", "device": "UDS100",
            "purpose": "query response incl. temp/humidity x1000 (3B) and overflow config low/high",
            "byteOrder": "bigEndian",
            "response": dev_response(2, [
                (0x01, bytes([0x02, 0x0A])), (0x8D, b"\x01"), (0x8B, bytes([0x0F, 0xA0])),
                (0x09, bytes([0x00, 0x61, 0xA8])), (0x0A, bytes([0x00, 0x2E, 0xE0])),
                (0x0E, bytes([0x00, 0x14])), (0x44, b"\x00"), (0x28, b"\x00"),
                (0x45, bytes([0x00, 0x3C])), (0x46, bytes([0x04, 0x38])), (0x47, b"\x00"),
                (0x48, bytes([0x01, 0x90, 0x0B, 0xB8])), (0x86, bytes([0x00, 0x1E])),
                (0x10, bytes([0x01, 0x02, 0x03, 0x04])), (0x11, bytes([0x05, 0x06, 0x07, 0x08])),
            ]),
            "expected": {
                "hardwareVersion": "0x02", "softwareVersion": "0x0A", "power": 1, "batteryVoltagemV": 4000,
                "temperatureMilli": 25000, "humidityMilli": 12000, "angle": 20, "slant": 0, "htAlarm": 0,
                "gpsPeriod": 60, "distance": 1080, "overflowState": 0,
                "lowThreshold": 400, "highThreshold": 3000, "reportPeriod": 30,
                "latitude": 16909060, "longitude": 84281096,
            },
            "firmwareReference": "UDS communicate.c callback_BLESearch + setDataPackage sizes",
            "iosTest": "TLVParserTests/testUDS100QueryResponse",
        },
        {
            "id": "RX-SVC-QUERY-001", "device": "SVC100",
            "purpose": "query response incl. dual-valve state(8B) and timezone 25=+03:30",
            "byteOrder": "bigEndian",
            "response": dev_response(3, [
                (0x01, bytes([0x0D, 0x0D])), (0x8D, b"\x01"), (0x03, b"\x63"),
                (0x3C, bytes([0x01, 0x00, 0x00, 0x64, 0x01, 0x01, 0x01, 0xF4])),
                (0x40, b"\x02"), (0x41, b"\x01"), (0x42, b"\x05"), (0x43, b"\x01"),
                (0x8A, b"\x19"), (0x86, bytes([0x00, 0x1E])),
            ]),
            "expected": {
                "hardwareVersion": "0x0D", "softwareVersion": "0x0D", "power": 1, "battery": 99,
                "valve1State": 1, "valve1Inserted": 0, "valve1Pulse": 100,
                "valve2State": 1, "valve2Inserted": 1, "valve2Pulse": 500,
                "voltageLevel": 2, "portFunction": 1, "stableTime": 5, "smartPower": 1,
                "timezone": "+03:30", "reportPeriod": 30,
            },
            "firmwareReference": "SVC communicate.c callback_BLEQuery + setDataPackage sizes + tz 25",
            "iosTest": "TLVParserTests/testSVC100QueryResponse",
        },
        {
            "id": "RX-ACK-001", "device": "ALL",
            "purpose": "bare ACK record after control commands",
            "response": dev_response(4, [(0xFF, b"\xFF")]),
            "expected": {"ack": True},
            "firmwareReference": "*/communicate.c callback_BLEAck",
            "iosTest": "TLVParserTests/testAckRecord",
        },
        {
            "id": "RX-UNKNOWN-TYPE-001", "device": "ALL",
            "purpose": "unknown type has no length field -> stream cannot be skipped safely -> data abnormal (S-6 revision)",
            "response": dev_response(5, [(0x01, bytes([0x0B, 0x1C])), (0x77, b"\x00"), (0x03, b"\x63")]),
            "expected": {"result": "dataAbnormal"},
            "firmwareReference": "setDataPackage has no length byte for any type",
            "iosTest": "TLVParserTests/testUnknownTypeDataAbnormal",
        },
    ]

    chunk0 = bytes(range(128))
    # Minimal padding to the 8-byte boundary, matching the firmware contract
    # ("最后一包不足 8 字节边界时，使用 FF 补齐"): 100 real bytes → 104 sent.
    last_real = bytes(range(100))
    last = last_real + bytes(8 - len(last_real) % 8)
    ota = [
        {
            "id": "OTA-NOTIFY-001", "device": "ALL",
            "purpose": "app-frame 0x01 notify -> device writes update flag and reboots into bootloader",
            "request": tx_frame(0x00, 0x01, FILL4),
            "firmwareReference": "*/communicate.c data[5]==1 branch: FLASH_Write_Update_Flag+reset",
            "iosTest": "OTAEngineTests/testNotify",
        },
        {
            "id": "OTA-FIRST-REQUEST-001", "device": "ALL",
            "purpose": "device ACK (cmd=0x02,count=0) requests FIRST packet number 0 - NOT 0x0002",
            "response": h([0x68, 0x6B, 0x74, 0x02, 0x00, 0x00]) + "626F6F746C6F6164",
            "expected": {"requestedPacket": 0},
            "firmwareReference": "BootLoader/uart.c case 1: InfoUartAck(cmd+1, flash_write_count=0)",
            "iosTest": "OTAEngineTests/testFirstRequestIsPacketZero",
        },
        {
            "id": "OTA-DATA-001", "device": "ALL",
            "purpose": "128-byte data packet, packet 0, 146-byte frame",
            "request": bt_frame(0x02, 0, chunk0),
            "expectedAck": h([0x68, 0x6B, 0x74, 0x02, 0x00, 0x01]) + "626F6F746C6F6164",
            "firmwareReference": "BootLoader/uart.c case 2: len-15==buflen, 128B fixed chunk",
            "iosTest": "OTATransferPlannerTests/testDataFrame",
        },
        {
            "id": "OTA-FINAL-001", "device": "ALL",
            "purpose": "final chunk 100B padded minimally with FF to the 8-byte boundary (104B sent)",
            "request": bt_frame(0x02, 5, last),
            "expectedAck": h([0x68, 0x6B, 0x74, 0x03, 0x00, 0x00]) + "626F6F746C6F6164",
            "firmwareReference": "BootLoader/uart.c: minimal FF padding; completion ACK cmd=3",
            "iosTest": "OTATransferPlannerTests/testFinalPadding",
        },
        {
            "id": "OTA-RESET-001", "device": "ALL",
            "purpose": "after ~10s silence device resets transfer and asks to start over: ACK(cmd=1,count=0)",
            "response": h([0x68, 0x6B, 0x74, 0x01, 0x00, 0x00]) + "626F6F746C6F6164",
            "expected": {"action": "restartFromPacketZero", "maxRestarts": 2},
            "firmwareReference": "BootLoader/uart.c reget_flash_data: recv_fail_cnt>10 -> reset+ACK(1,0)",
            "iosTest": "OTAEngineTests/testDeviceResetRestart",
        },
    ]

    Path("app-frame-tx.json").write_text(json.dumps(tx, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    Path("response-parse.json").write_text(json.dumps(resp, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    Path("ota-transfer.json").write_text(json.dumps(ota, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print("wrote app-frame-tx.json, response-parse.json, ota-transfer.json")


if __name__ == "__main__":
    main()
