#pragma once
// Thin wrapper over the IridiumSBD library. Only this file talks to the modem.
#include <IridiumSBD.h>  // for ISBD_SUCCESS and the other ISBD_* error codes
#include <stddef.h>
#include <stdint.h>

bool iridiumBegin();  // drives SLEEP pin high, waits for the modem to answer "AT"

// One satellite session (AT+SBDWB + AT+SBDIX [+ AT+SBDRB]).
// tx == nullptr -> receive-only "mailbox check".
// rxLen: in = buffer size, out = bytes received (0 = no downlink).
// waitingMt: downlinks still queued at the gateway after this session.
// Returns ISBD_SUCCESS or an ISBD_* error code.
int iridiumSession(const uint8_t* tx, size_t txLen, uint8_t* rx, size_t& rxLen, int& waitingMt);

int  iridiumSignalQuality();      // 0..5 bars, -1 on error
bool iridiumNetworkAvailable();   // NA pin
bool iridiumRingAsserted();       // a downlink is waiting (only works with KEEP_MODEM_AWAKE)
void iridiumSleep();
