//setup() + main loop: runs the state machine
// WEC float firmware — top-level state machine.
// Each wake: SAMPLE -> BUILD -> SEND (Iridium) -> [STARLINK] -> SLEEP (deep sleep, then the ESP32 reboots into setup()).
#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include "commands.h"
#include "config.h"
#include "crypto.h"
#include "iridium_link.h"
#include "link_manager.h"
#include "packet.h"
#include "sensors.h"
#include "storage.h"

enum class State { SAMPLE, BUILD, SEND, STARLINK, SLEEP };

static State         state = State::SAMPLE;
static SensorReading reading;
static uint8_t       pkt[MO_MAX];
static size_t        pktLen = 0;
RTC_DATA_ATTR static uint32_t wakeCount = 0;  // RTC_DATA_ATTR = survives deep sleep

static Telemetry toTelemetry(const SensorReading& r) {
  Telemetry t{};
  t.unix_time      = r.unixTime;
  t.lat_e7         = r.lat_e7;
  t.lon_e7         = r.lon_e7;
  t.tilt_cdeg      = static_cast<int16_t>(lroundf(r.tiltDeg * 100));
  t.batt_mv        = r.battMv;
  t.temp_cC        = static_cast<int16_t>(lroundf(r.tempC * 100));
  t.wec_power_dW   = static_cast<uint16_t>(constrain(lroundf(r.wecPowerW * 10), 0L, 65535L));
  t.wave_height_mm = static_cast<uint16_t>(constrain(lroundf(r.waveHeightM * 1000), 0L, 65535L));
  t.flags          = (r.gpsValid ? FLAG_GPS_VALID : 0) | (r.imuOk ? FLAG_IMU_OK : 0) |
                     (storageSdOk() ? FLAG_SD_OK : 0);
  t.queued         = storageQueueCount();
  return t;
}

static void goToSleep() {
  if (rebootRequested()) ESP.restart();

  uint32_t interval = reportIntervalS();
  if (!KEEP_MODEM_AWAKE) iridiumSleep();
  digitalWrite(PIN_STARLINK_EN, LOW);

  // Without a hold, pins float during deep sleep and could switch the modem or Starlink on.
  gpio_hold_en(static_cast<gpio_num_t>(PIN_IRD_SLEEP));
  gpio_hold_en(static_cast<gpio_num_t>(PIN_STARLINK_EN));
  gpio_deep_sleep_hold_en();

  esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(interval) * 1000000ULL);
  if (KEEP_MODEM_AWAKE) {
    esp_sleep_enable_ext0_wakeup(PIN_IRD_RI, 0);  // RI is active-low on the 9603: check your board
  }
  Serial.printf("Sleeping %lu s\n", (unsigned long)interval);
  Serial.flush();
  esp_deep_sleep_start();  // does not return
}

void setup() {
  Serial.begin(115200);
  gpio_hold_dis(static_cast<gpio_num_t>(PIN_IRD_SLEEP));
  gpio_hold_dis(static_cast<gpio_num_t>(PIN_STARLINK_EN));
  pinMode(PIN_STARLINK_EN, OUTPUT);
  digitalWrite(PIN_STARLINK_EN, LOW);

  wakeCount++;
  Serial.printf("\n=== Wake #%lu, cause %d ===\n", (unsigned long)wakeCount,
                (int)esp_sleep_get_wakeup_cause());

  cryptoBegin();
  commandsBegin();
  storageBegin();
  sensorsBegin();
}

void loop() {
  switch (state) {
    case State::SAMPLE:
      sensorsRead(reading, GPS_FIX_TIMEOUT_MS);
      storageLog(reading);
      state = State::BUILD;
      break;

    case State::BUILD: {
      Telemetry t = toTelemetry(reading);
      pktLen = buildPacket(MSG_TELEMETRY, &t, sizeof t, pkt, sizeof pkt);
      state = pktLen ? State::SEND : State::SLEEP;
      break;
    }

    case State::SEND:
      linkSendIridium(pkt, pktLen, commandsHandleMt);  // new report first
      linkFlushQueue(commandsHandleMt);                // then old failures + ACKs
      state = starlinkRequestedMinutes() ? State::STARLINK : State::SLEEP;
      break;

    case State::STARLINK:
      linkStarlinkSession(starlinkRequestedMinutes());
      clearStarlinkRequest();
      state = State::SLEEP;
      break;

    case State::SLEEP:
      goToSleep();
      break;
  }
}
