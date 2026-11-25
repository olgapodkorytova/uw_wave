#pragma once
// Unsent-packet queue + sensor log.
// With an SD card: queue is files in /q/ (survives power loss), log is /log.csv.
// Without one: queue falls back to a small ring buffer in RTC memory (survives deep sleep, not power loss).
#include <stddef.h>
#include <stdint.h>
#include "sensors.h"

bool    storageBegin();  // true if SD card mounted
bool    storageSdOk();
bool    storageQueuePush(const uint8_t* pkt, size_t len, uint32_t counter);
size_t  storageQueuePeek(uint8_t* out, size_t max, uint32_t& counter);  // oldest packet; 0 if empty
void    storageQueuePop(uint32_t counter);
uint8_t storageQueueCount();
void    storageLog(const SensorReading& r);
