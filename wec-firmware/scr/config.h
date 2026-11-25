//pins, report interval, retry limits, keys (not in git!)
#pragma once
// All pins and tuning numbers live here so nothing else hard-codes them.
#include <Arduino.h>

// ---------------- Pins (ESP32 DevKit / WROOM) ----------------
// Iridium modem (RockBLOCK 9603 / SparkFun 9603N) on UART2
constexpr int        PIN_IRD_RX    = 16;           // ESP32 RX2  <- modem TXD
constexpr int        PIN_IRD_TX    = 17;           // ESP32 TX2  -> modem RXD
constexpr int        PIN_IRD_SLEEP = 25;           // -> modem OnOff/SLEEP (HIGH = modem on). RTC GPIO so it can be held in deep sleep
constexpr gpio_num_t PIN_IRD_RI    = GPIO_NUM_33;  // <- modem Ring Indicator. RTC GPIO so it can wake the ESP32
constexpr int        PIN_IRD_NA    = 34;           // <- modem Network Available (input-only pin)

// Starlink Mini load switch
constexpr int PIN_STARLINK_EN = 26;  // -> load switch EN (HIGH = Starlink powered). Add a pull-down resistor in hardware!

// GNSS on UART1 (TX/RX remapped away from the flash pins)
constexpr int      PIN_GPS_RX = 32;  // ESP32 RX1 <- GNSS TX
constexpr int      PIN_GPS_TX = 27;  // ESP32 TX1 -> GNSS RX
constexpr uint32_t GPS_BAUD   = 9600;

// I2C (IMU)
constexpr int PIN_I2C_SDA = 21;
constexpr int PIN_I2C_SCL = 22;

// Battery voltage divider -> ADC (input-only pin)
constexpr int   PIN_VBAT_ADC = 35;
constexpr float VBAT_DIVIDER = 4.0f;  // e.g. 30k/10k divider: Vbat = Vadc * 4. Match your resistors.

// MicroSD (VSPI)
constexpr int PIN_SD_MOSI = 23;
constexpr int PIN_SD_MISO = 19;
constexpr int PIN_SD_SCK  = 18;
constexpr int PIN_SD_CS   = 5;

// ---------------- Behaviour ----------------
constexpr uint32_t DEFAULT_REPORT_INTERVAL_S = 3600;  // 1 report/hour; changeable by command
constexpr uint32_t MIN_REPORT_INTERVAL_S     = 300;
constexpr uint32_t MAX_REPORT_INTERVAL_S     = 86400;

constexpr uint32_t GPS_FIX_TIMEOUT_MS   = 60000;  // give up on GNSS after this; send without position

constexpr float    UPRIGHT_MAX_TILT_DEG = 20.0f;   // antenna axis within 20 deg of vertical
constexpr uint32_t UPRIGHT_HOLD_MS      = 3000;    // ...continuously for 3 s
constexpr uint32_t UPRIGHT_TIMEOUT_MS   = 120000;  // stop waiting after 2 min

constexpr int      IRD_SESSION_TIMEOUT_S = 120;  // library keeps retrying SBDIX inside this window
constexpr int      IRD_MAX_ATTEMPTS      = 3;    // our own outer retries (each waits for upright first)
constexpr uint32_t IRD_BACKOFF_MS        = 15000;// attempt n waits n * this before retrying
constexpr int      MAX_MT_PER_WAKE       = 3;    // downlinks to pull per wake (each session costs credits)
constexpr int      MAX_QUEUE_PER_WAKE    = 3;    // queued uplinks to resend per wake

// true  = modem stays powered between reports, RI pin wakes the ESP32 instantly when a command arrives (more power)
// false = modem fully asleep; commands are only picked up at the next scheduled report ("mailbox check")
constexpr bool KEEP_MODEM_AWAKE = false;

constexpr uint32_t STARLINK_BOOT_TIMEOUT_MS = 180000;  // Starlink takes minutes to boot + acquire
constexpr uint16_t STARLINK_MAX_MINUTES     = 30;

#ifndef IRIDIUM_DEBUG
#define IRIDIUM_DEBUG 0
#endif
