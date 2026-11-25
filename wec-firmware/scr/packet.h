//struct → packed bytes (and command bytes → struct)
#pragma once
// Byte layout of every message. ground/decode.py MUST mirror these structs exactly.
// ESP32 is little-endian; Python uses "<" formats.
#include <stdint.h>
#include <stddef.h>

constexpr uint8_t PROTO_VERSION = 1;

enum MsgType : uint8_t {
  MSG_TELEMETRY = 0x01,  // float -> ground
  MSG_COMMAND   = 0x02,  // ground -> float
  MSG_ACK       = 0x03,  // float -> ground, reply to a command
};

// Sent in the clear but authenticated (GCM "additional data"), so it can't be altered.
struct __attribute__((packed)) PacketHeader {
  uint8_t  version;
  uint8_t  type;     // MsgType
  uint32_t counter;  // per-direction message counter = GCM nonce. Never repeats.
};
static_assert(sizeof(PacketHeader) == 6, "header size changed: update decode.py");

// Python: struct.Struct("<IiihHhHHBB")
struct __attribute__((packed)) Telemetry {
  uint32_t unix_time;       // from GNSS, 0 if no fix
  int32_t  lat_e7;          // degrees * 1e7
  int32_t  lon_e7;
  int16_t  tilt_cdeg;       // degrees * 100
  uint16_t batt_mv;
  int16_t  temp_cC;         // deg C * 100
  uint16_t wec_power_dW;    // W * 10
  uint16_t wave_height_mm;
  uint8_t  flags;           // FLAG_* below
  uint8_t  queued;          // packets waiting in the unsent queue
};
static_assert(sizeof(Telemetry) == 24, "telemetry size changed: update decode.py");

enum TelemetryFlags : uint8_t {
  FLAG_GPS_VALID = 1 << 0,
  FLAG_IMU_OK    = 1 << 1,
  FLAG_SD_OK     = 1 << 2,
};

// Python: struct.Struct("<BI")
struct __attribute__((packed)) Command {
  uint8_t  cmd_id;  // CmdId
  uint32_t arg;
};

enum CmdId : uint8_t {
  CMD_PING             = 1,  // arg unused, float just ACKs
  CMD_SET_INTERVAL     = 2,  // arg = seconds between reports
  CMD_STARLINK_SESSION = 3,  // arg = max minutes to power Starlink and dump the queue
  CMD_REBOOT           = 4,
};

// Python: struct.Struct("<IBB")
struct __attribute__((packed)) Ack {
  uint32_t acked_counter;  // counter of the command being acknowledged
  uint8_t  cmd_id;
  uint8_t  result;         // 0 = ok, 1 = bad argument, 2 = unknown command
};

constexpr size_t MO_MAX   = 340;  // Iridium SBD uplink limit (bytes)
constexpr size_t MT_MAX   = 270;  // Iridium SBD downlink limit
constexpr size_t TAG_LEN  = 16;   // AES-GCM authentication tag
constexpr size_t OVERHEAD = sizeof(PacketHeader) + TAG_LEN;  // 22 bytes -> 318 bytes of real data per MO

// header | encrypted body | tag. Returns total length, 0 on error.
size_t buildPacket(uint8_t type, const void* body, size_t bodyLen, uint8_t* out, size_t outMax);

// Checks version, authenticity and replay. Returns body length, -1 if rejected.
int parsePacket(const uint8_t* in, size_t inLen, PacketHeader& hdr, uint8_t* bodyOut, size_t bodyMax);

// Reads the counter out of an already-built packet.
uint32_t packetCounter(const uint8_t* pkt);
