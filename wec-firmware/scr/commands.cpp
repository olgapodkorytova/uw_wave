//handles downlink commands (change interval, Starlink on…)

#include "commands.h"
#include <Preferences.h>
#include <string.h>
#include "config.h"
#include "packet.h"
#include "storage.h"

static Preferences cfg;
static uint16_t    starlinkMinutes = 0;
static bool        rebootFlag = false;

void commandsBegin() { cfg.begin("cfg", false); }

uint32_t reportIntervalS() { return cfg.getUInt("interval", DEFAULT_REPORT_INTERVAL_S); }
uint16_t starlinkRequestedMinutes() { return starlinkMinutes; }
void     clearStarlinkRequest() { starlinkMinutes = 0; }
bool     rebootRequested() { return rebootFlag; }

static void queueAck(uint32_t ackedCounter, uint8_t cmdId, uint8_t result) {
  Ack     a{ackedCounter, cmdId, result};
  uint8_t pkt[MO_MAX];
  size_t  n = buildPacket(MSG_ACK, &a, sizeof a, pkt, sizeof pkt);
  if (n) storageQueuePush(pkt, n, packetCounter(pkt));  // goes out with the next flush
}

void commandsHandleMt(const uint8_t* mt, size_t len) {
  PacketHeader hdr;
  uint8_t      body[MT_MAX];
  int          n = parsePacket(mt, len, hdr, body, sizeof body);
  if (n < 0 || hdr.type != MSG_COMMAND || n != static_cast<int>(sizeof(Command))) {
    Serial.printf("[cmd] rejected MT (%u bytes): bad auth, replay, or format\n", (unsigned)len);
    return;
  }

  Command c;
  memcpy(&c, body, sizeof c);
  uint8_t result = 0;

  switch (c.cmd_id) {
    case CMD_PING:
      break;
    case CMD_SET_INTERVAL:
      if (c.arg >= MIN_REPORT_INTERVAL_S && c.arg <= MAX_REPORT_INTERVAL_S) cfg.putUInt("interval", c.arg);
      else result = 1;
      break;
    case CMD_STARLINK_SESSION:
      if (c.arg >= 1 && c.arg <= STARLINK_MAX_MINUTES) starlinkMinutes = c.arg;
      else result = 1;
      break;
    case CMD_REBOOT:
      rebootFlag = true;
      break;
    default:
      result = 2;
  }
  Serial.printf("[cmd] #%lu id=%u arg=%lu -> result %u\n",
                (unsigned long)hdr.counter, c.cmd_id, (unsigned long)c.arg, result);
  queueAck(hdr.counter, c.cmd_id, result);
}
