// ============================================================
// config.h — iGate APRS TI2TEC
// LILYGO T3 V1.6.1
// ============================================================
#pragma once

// ── Identificación APRS ──────────────────────────────────────
#define CALLSIGN        "TI1TEC"
#define SSID_APRS       "TI1TEC-10"
#define PASSCODE        "21436"
#define APRS_COMMENT    "LoRa iGate CR"

// ── Red WiFi ─────────────────────────────────────────────────
#define WIFI_SSID       "S23 FE de Alvaro"
#define WIFI_PASSWORD   "lolis999"
#define WIFI_TIMEOUT_MS  20000

// ── Servidor APRS-IS ─────────────────────────────────────────
#define APRSIS_SERVER   "rotate.aprs2.net"
#define APRSIS_PORT      14580
#define APRSIS_FILTER   "r/10.02/-84.06/200"

// ── Posición fija (sin GPS) ───────────────────────────────────
#define BEACON_LAT       10.0222
#define BEACON_LON      -84.0560
#define BEACON_ALT       1200
#define BEACON_INTERVAL  600000
#define BEACON_SYMBOL   "/&"

// ── LoRa SX1276 — pines LILYGO T3 V1.6.1 ────────────────────
#define LORA_FREQ        433.775E6
#define LORA_BW          125.0E3
#define LORA_SF          12
#define LORA_CR          5
#define LORA_SYNC        0x12
#define LORA_POWER       17
#define LORA_PREAMBLE    8

#define LORA_SCK         5
#define LORA_MISO        19
#define LORA_MOSI        27
#define LORA_CS          18
#undef  LORA_RST
#define LORA_RST         14
#define LORA_DIO0        26
#define LORA_DIO1        33

// ── OLED SSD1306 — pines LILYGO T3 V1.6.1 ───────────────────
#undef  OLED_SDA
#undef  OLED_SCL
#define OLED_SDA         21
#define OLED_SCL         22
#define OLED_ADDR        0x3C

// ── Watchdog ─────────────────────────────────────────────────
#define WDT_TIMEOUT_S    30

// ── Versión firmware ─────────────────────────────────────────
#define FW_VERSION       "1.0.0"