#pragma once
// Handles downlink (MT) commands and holds the settings they change.
#include <stddef.h>
#include <stdint.h>

void     commandsBegin();
void     commandsHandleMt(const uint8_t* mt, size_t len);  // matches MtHandler

uint32_t reportIntervalS();           // saved in flash, survives power loss
uint16_t starlinkRequestedMinutes();  // 0 = no Starlink session requested this wake
void     clearStarlinkRequest();
bool     rebootRequested();
