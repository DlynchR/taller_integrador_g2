 // display.h — manejo del OLED SSD1306 128x64 vía I2C
 #pragma once

#include <Arduino.h>
#include "aprs_parser.h"

// Inicializa el OLED y muestra pantalla de arranque
void display_init();

// Pantalla de arranque - muestra callsign y versión
void display_boot();

// Pantalla de conexión WiFi
void display_wifi_connecting();
void display_wifi_connected(const String& ip);
void display_wifi_failed();

// Pantalla de conexión APRS-IS
void display_aprsis_connecting();
void display_aprsis_connected();
void display_aprsis_failed();

// Pantalla principal - estado normal de operación (S3)
// Muestra: callsign, WiFi, APRS-IS, contadores
void display_status(uint32_t rx_count, uint32_t tx_count);

// Pantalla de paquete recibido — muestra src, RSSI, SNR
void display_packet(const AprsFrame& frame);

// Pantalla de beacon TX
void display_beacon();