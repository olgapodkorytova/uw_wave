'''
The most important rule across both programs: 
packet.cpp and decode.py must define exactly 
the same byte layout. Put a version byte at the 
start of every packet so you can change the format 
later without breaking old data.
'''
"""Packet format — mirror of firmware/src/packet.h. Change one, change the other."""
import struct

from crypto import DIR_MO, DIR_MT, decrypt, encrypt

PROTO_VERSION = 1
MSG_TELEMETRY, MSG_COMMAND, MSG_ACK = 0x01, 0x02, 0x03

HEADER = struct.Struct("<BBI")             # PacketHeader: version, type, counter
TELEMETRY = struct.Struct("<IiihHhHHBB")   # Telemetry (24 bytes)
COMMAND = struct.Struct("<BI")             # Command: cmd_id, arg
ACK = struct.Struct("<IBB")                # Ack: acked_counter, cmd_id, result
TAG_LEN = 16
MO_MAX, MT_MAX = 340, 270

assert TELEMETRY.size == 24 and HEADER.size == 6

CMD_IDS = {"ping": 1, "set-interval": 2, "starlink": 3, "reboot": 4}
CMD_NAMES = {v: k for k, v in CMD_IDS.items()}
ACK_RESULTS = {0: "ok", 1: "bad argument", 2: "unknown command"}

FLAG_GPS_VALID, FLAG_IMU_OK, FLAG_SD_OK = 1, 2, 4


def decode_packet(raw: bytes) -> dict:
    """Float -> ground packet. Raises ValueError if malformed or not authentic."""
    if len(raw) < HEADER.size + TAG_LEN:
        raise ValueError(f"too short ({len(raw)} bytes)")
    header = raw[:HEADER.size]
    version, mtype, counter = HEADER.unpack(header)
    if version != PROTO_VERSION:
        raise ValueError(f"unknown protocol version {version}")

    body = decrypt(DIR_MO, counter, header, raw[HEADER.size:])
    out = {"counter": counter}

    if mtype == MSG_TELEMETRY and len(body) == TELEMETRY.size:
        (t, lat, lon, tilt, batt, temp, power, wave, flags, queued) = TELEMETRY.unpack(body)
        gps = bool(flags & FLAG_GPS_VALID)
        out.update(
            type="telemetry",
            unix_time=t or None,
            lat=lat / 1e7 if gps else None,
            lon=lon / 1e7 if gps else None,
            tilt_deg=tilt / 100,
            battery_v=batt / 1000,
            temp_c=temp / 100,
            wec_power_w=power / 10,
            wave_height_m=wave / 1000,
            gps_valid=gps,
            imu_ok=bool(flags & FLAG_IMU_OK),
            sd_ok=bool(flags & FLAG_SD_OK),
            queued=queued,
        )
    elif mtype == MSG_ACK and len(body) == ACK.size:
        acked, cmd_id, result = ACK.unpack(body)
        out.update(
            type="ack",
            acked_counter=acked,
            command=CMD_NAMES.get(cmd_id, cmd_id),
            result=ACK_RESULTS.get(result, result),
        )
    else:
        raise ValueError(f"unexpected type {mtype} / body length {len(body)}")
    return out


def encode_command(counter: int, cmd_name: str, arg: int = 0) -> bytes:
    """Ground -> float packet."""
    header = HEADER.pack(PROTO_VERSION, MSG_COMMAND, counter)
    pkt = header + encrypt(DIR_MT, counter, header, COMMAND.pack(CMD_IDS[cmd_name], arg))
    if len(pkt) > MT_MAX:
        raise ValueError("MT packet too long")
    return pkt


def encode_telemetry_for_test(counter: int, **f) -> bytes:
    """Builds a packet exactly like the float would. Used by fake_float.py for testing without hardware."""
    flags = FLAG_GPS_VALID | FLAG_IMU_OK
    body = TELEMETRY.pack(
        f.get("unix_time", 0), round(f.get("lat", 0) * 1e7), round(f.get("lon", 0) * 1e7),
        round(f.get("tilt_deg", 0) * 100), round(f.get("battery_v", 0) * 1000),
        round(f.get("temp_c", 0) * 100), round(f.get("wec_power_w", 0) * 10),
        round(f.get("wave_height_m", 0) * 1000), flags, f.get("queued", 0),
    )
    header = HEADER.pack(PROTO_VERSION, MSG_TELEMETRY, counter)
    return header + encrypt(DIR_MO, counter, header, body)
