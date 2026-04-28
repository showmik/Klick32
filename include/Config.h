#pragma once

// ─── Pin Map ──────────────────────────────────────────────────────────────────

// I2C Display
#define PIN_SDA        8
#define PIN_SCL        9

// D-pad
#define PIN_UP        13
#define PIN_DOWN      14
#define PIN_LEFT      12
#define PIN_RIGHT     11

// Action buttons
#define PIN_BTN_A     16
#define PIN_BTN_B     15

// Menu buttons
#define PIN_MENU1      5
#define PIN_MENU2      4

// ── Peripherals (wire these up before enabling) ───────────────────────────────
#define PIN_BUZZER    17   // Passive buzzer (signal pin)
#define PIN_BATT_ADC   2   // Battery voltage — connect via 100k/100k divider
#define PIN_CHRG      38   // Charging IC CHRG pin (active LOW); tie to 3.3V if unused

// ─── Display ─────────────────────────────────────────────────────────────────
#define SCREEN_W     128
#define SCREEN_H      64

// ─── Firmware Identity ────────────────────────────────────────────────────────
// Change FW_NAME to whatever you end up calling your console.
#define FW_NAME      "GADGETBOY"
#define FW_VERSION   "v0.1"

// ─── Timing ──────────────────────────────────────────────────────────────────
#define FRAME_MS      33    // ~30 fps target frame budget (ms)