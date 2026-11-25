#include "sensors.h"
#include <Wire.h>
#include <TinyGPSPlus.h>
#include <Adafruit_BNO055.h>
#include <time.h>
#include "config.h"

static TinyGPSPlus     gps;
static HardwareSerial& GpsSerial = Serial1;
static Adafruit_BNO055 bno(55, 0x28, &Wire);
static bool            imuOk = false;

void sensorsBegin() {
  GpsSerial.begin(GPS_BAUD, SERIAL_8N1, PIN_GPS_RX, PIN_GPS_TX);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  imuOk = bno.begin();
  if (!imuOk) Serial.println("IMU not found");
  analogSetPinAttenuation(PIN_VBAT_ADC, ADC_11db);
}

void gpsFeed() {
  while (GpsSerial.available()) gps.encode(GpsSerial.read());
}

float imuTiltDeg() {
  if (!imuOk) return 0;
  // Assumes the IMU's Z axis points along the antenna. Change the axis if you mount it differently.
  imu::Vector<3> g = bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);
  float mag = sqrtf(g.x() * g.x() + g.y() * g.y() + g.z() * g.z());
  if (mag < 1.0f) return 90.0f;  // bad reading: treat as "not upright"
  return degrees(acosf(constrain(g.z() / mag, -1.0f, 1.0f)));
}

static uint32_t gpsUnixTime() {
  struct tm t = {};
  t.tm_year = gps.date.year() - 1900;
  t.tm_mon  = gps.date.month() - 1;
  t.tm_mday = gps.date.day();
  t.tm_hour = gps.time.hour();
  t.tm_min  = gps.time.minute();
  t.tm_sec  = gps.time.second();
  return static_cast<uint32_t>(mktime(&t));  // ESP32 default time zone is UTC
}

// ---- WEC-specific sensors: replace these stubs with your real hardware ----
static float readWecPowerW() {
  // TODO: e.g. INA226 on the generator output, or a current shunt into the ADC
  return 0.0f;
}
static float readWaveHeightM() {
  // TODO: e.g. significant wave height from IMU vertical acceleration over a sampling window
  return 0.0f;
}

void sensorsRead(SensorReading& r, uint32_t gpsTimeoutMs) {
  uint32_t start = millis();
  while (millis() - start < gpsTimeoutMs) {
    gpsFeed();
    if (gps.location.isValid() && gps.location.age() < 2000 && gps.date.isValid()) break;
    delay(10);
  }
  r.gpsValid = gps.location.isValid() && gps.date.isValid();
  if (r.gpsValid) {
    r.lat_e7   = static_cast<int32_t>(lround(gps.location.lat() * 1e7));
    r.lon_e7   = static_cast<int32_t>(lround(gps.location.lng() * 1e7));
    r.unixTime = gpsUnixTime();
  }

  r.imuOk   = imuOk;
  r.tiltDeg = imuTiltDeg();
  r.tempC   = imuOk ? bno.getTemp() : 0;  // BNO055 die temperature; swap for a water-temp sensor if you have one
  r.battMv  = static_cast<uint16_t>(analogReadMilliVolts(PIN_VBAT_ADC) * VBAT_DIVIDER);
  r.wecPowerW   = readWecPowerW();
  r.waveHeightM = readWaveHeightM();
}
