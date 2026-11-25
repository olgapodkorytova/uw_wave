//optional microSD log / unsent-message queue
#include "storage.h"
#include <SD.h>
#include <SPI.h>
#include "config.h"
#include "packet.h"

static bool sdOk = false;

// ---- RTC-memory fallback queue ----
constexpr uint8_t RTC_SLOTS = 4;
RTC_DATA_ATTR static uint8_t  rtcBuf[RTC_SLOTS][MO_MAX];
RTC_DATA_ATTR static uint16_t rtcLen[RTC_SLOTS];
RTC_DATA_ATTR static uint8_t  rtcHead = 0, rtcCount = 0;

static String qPath(uint32_t counter) {
  char name[24];
  snprintf(name, sizeof name, "/q/%010lu.bin", (unsigned long)counter);
  return String(name);
}

bool storageBegin() {
  SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  sdOk = SD.begin(PIN_SD_CS);
  if (sdOk && !SD.exists("/q")) SD.mkdir("/q");
  if (!sdOk) Serial.println("[storage] no SD card, using RTC queue");
  return sdOk;
}

bool storageSdOk() { return sdOk; }

bool storageQueuePush(const uint8_t* pkt, size_t len, uint32_t counter) {
  if (len > MO_MAX) return false;
  if (sdOk) {
    File f = SD.open(qPath(counter), FILE_WRITE);
    if (!f) return false;
    f.write(pkt, len);
    f.close();
    return true;
  }
  if (rtcCount == RTC_SLOTS) {  // full: drop the oldest
    rtcHead = (rtcHead + 1) % RTC_SLOTS;
    rtcCount--;
  }
  uint8_t i = (rtcHead + rtcCount) % RTC_SLOTS;
  memcpy(rtcBuf[i], pkt, len);
  rtcLen[i] = len;
  rtcCount++;
  return true;
}

size_t storageQueuePeek(uint8_t* out, size_t max, uint32_t& counter) {
  if (sdOk) {
    // Oldest = lowest counter in the file name.
    File     dir = SD.open("/q");
    uint32_t best = UINT32_MAX;
    for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
      uint32_t c = strtoul(f.name(), nullptr, 10);
      if (c > 0 && c < best) best = c;
      f.close();
    }
    dir.close();
    if (best == UINT32_MAX) return 0;
    File f = SD.open(qPath(best));
    size_t n = f ? f.read(out, max) : 0;
    if (f) f.close();
    counter = best;
    return n;
  }
  if (rtcCount == 0 || rtcLen[rtcHead] > max) return 0;
  memcpy(out, rtcBuf[rtcHead], rtcLen[rtcHead]);
  counter = packetCounter(out);
  return rtcLen[rtcHead];
}

void storageQueuePop(uint32_t counter) {
  if (sdOk) {
    SD.remove(qPath(counter));
    return;
  }
  if (rtcCount == 0) return;
  rtcHead = (rtcHead + 1) % RTC_SLOTS;
  rtcCount--;
}

uint8_t storageQueueCount() {
  if (!sdOk) return rtcCount;
  uint8_t n = 0;
  File    dir = SD.open("/q");
  for (File f = dir.openNextFile(); f && n < 255; f = dir.openNextFile()) {
    n++;
    f.close();
  }
  dir.close();
  return n;
}

void storageLog(const SensorReading& r) {
  if (!sdOk) return;
  File f = SD.open("/log.csv", FILE_APPEND);
  if (!f) return;
  f.printf("%lu,%ld,%ld,%.1f,%u,%.2f,%.1f,%.2f\n", (unsigned long)r.unixTime, (long)r.lat_e7,
           (long)r.lon_e7, r.tiltDeg, r.battMv, r.tempC, r.wecPowerW, r.waveHeightM);
  f.close();
}
