//read IMU, GNSS, WEC sensors → fills a struct
#pragma once
#include <stdint.h>

struct SensorReading {
  bool     gpsValid = false;
  int32_t  lat_e7 = 0, lon_e7 = 0;
  uint32_t unixTime = 0;
  bool     imuOk = false;
  float    tiltDeg = 0;
  float    tempC = 0;
  uint16_t battMv = 0;
  float    wecPowerW = 0;
  float    waveHeightM = 0;
};

void  sensorsBegin();
void  sensorsRead(SensorReading& r, uint32_t gpsTimeoutMs);  // blocks up to gpsTimeoutMs for a fix
float imuTiltDeg();  // antenna tilt from vertical; 0 if no IMU (so we still try to transmit)
void  gpsFeed();     // call often while doing anything slow, or GNSS bytes get dropped
