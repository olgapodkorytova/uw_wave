//wraps IridiumSBD: wake, send, receive, sleep
#include "iridium_link.h"
#include "config.h"
#include "sensors.h"

static HardwareSerial& IridiumSerial = Serial2;
static IridiumSBD      modem(IridiumSerial, PIN_IRD_SLEEP, static_cast<int>(PIN_IRD_RI));

// The library calls this repeatedly during long operations (a session can take minutes).
// Keep it short. Returning false aborts the session.
bool ISBDCallback() {
  gpsFeed();
  return true;
}

#if IRIDIUM_DEBUG
// Print the raw AT conversation; this is how you learn what the modem is doing.
void ISBDConsoleCallback(IridiumSBD*, char c) { Serial.write(c); }
void ISBDDiagsCallback(IridiumSBD*, char c) { Serial.write(c); }
#endif

bool iridiumBegin() {
  pinMode(PIN_IRD_NA, INPUT);
  IridiumSerial.begin(19200, SERIAL_8N1, PIN_IRD_RX, PIN_IRD_TX);  // 9602/9603 default baud

  modem.setPowerProfile(IridiumSBD::DEFAULT_POWER_PROFILE);  // use USB_POWER_PROFILE if your supply is current-limited (~450 mA)
  modem.adjustSendReceiveTimeout(IRD_SESSION_TIMEOUT_S);

  int err = modem.begin();
  if (err != ISBD_SUCCESS) {
    Serial.printf("[iridium] begin failed: %d\n", err);
    return false;
  }
  modem.enableRingAlerts(KEEP_MODEM_AWAKE);
  return true;
}

int iridiumSession(const uint8_t* tx, size_t txLen, uint8_t* rx, size_t& rxLen, int& waitingMt) {
  int err = tx ? modem.sendReceiveSBDBinary(tx, txLen, rx, rxLen)
               : modem.sendReceiveSBDText(nullptr, rx, rxLen);  // receive-only
  if (err != ISBD_SUCCESS) rxLen = 0;
  waitingMt = (err == ISBD_SUCCESS) ? modem.getWaitingMessageCount() : 0;
  return err;
}

int iridiumSignalQuality() {
  int q = -1;
  return modem.getSignalQuality(q) == ISBD_SUCCESS ? q : -1;
}

bool iridiumNetworkAvailable() { return digitalRead(PIN_IRD_NA) == HIGH; }

bool iridiumRingAsserted() { return modem.hasRingAsserted(); }

void iridiumSleep() { modem.sleep(); }
