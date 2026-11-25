#include "packet.h"
#include <string.h>
#include "crypto.h"

size_t buildPacket(uint8_t type, const void* body, size_t bodyLen, uint8_t* out, size_t outMax) {
  const size_t total = sizeof(PacketHeader) + bodyLen + TAG_LEN;
  if (total > outMax || total > MO_MAX) return 0;

  PacketHeader h{PROTO_VERSION, type, nextTxCounter()};  // counter is persisted before use
  memcpy(out, &h, sizeof h);

  uint8_t* ct  = out + sizeof h;
  uint8_t* tag = ct + bodyLen;
  if (!gcmEncrypt(DIR_MO, h.counter, out, sizeof h,
                  static_cast<const uint8_t*>(body), bodyLen, ct, tag)) {
    return 0;
  }
  return total;
}

int parsePacket(const uint8_t* in, size_t inLen, PacketHeader& h, uint8_t* bodyOut, size_t bodyMax) {
  if (inLen < OVERHEAD) return -1;
  memcpy(&h, in, sizeof h);
  if (h.version != PROTO_VERSION) return -1;

  const size_t n = inLen - OVERHEAD;
  if (n > bodyMax) return -1;

  const uint8_t* ct  = in + sizeof h;
  const uint8_t* tag = ct + n;
  if (!gcmDecrypt(DIR_MT, h.counter, in, sizeof h, ct, n, tag, bodyOut)) return -1;  // forged/corrupt

  // Replay check AFTER authentication, so a forged packet can't push the stored counter forward.
  if (!acceptRxCounter(h.counter)) return -1;
  return static_cast<int>(n);
}

uint32_t packetCounter(const uint8_t* pkt) {
  PacketHeader h;
  memcpy(&h, pkt, sizeof h);
  return h.counter;
}
