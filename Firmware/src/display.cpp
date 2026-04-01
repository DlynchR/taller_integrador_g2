 // display.cpp — manejo del OLED SSD1306 128x64 vía I2C
 #include "display.h"
#include "config.h"

#include <Wire.h>
#include <U8g2lib.h>

//   Instancia U8g2  
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C
    u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);

//   display_init   
void display_init() {
    Wire.begin(OLED_SDA, OLED_SCL);
    u8g2.begin();
    u8g2.setContrast(200);
    display_boot();
}

//   display_boot   
void display_boot() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_7x13B_tr);
    u8g2.drawStr(20, 20, "LoRa iGate");

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(28, 36, SSID_APRS);
    u8g2.drawStr(28, 50, "v" FW_VERSION);

    u8g2.drawHLine(0, 56, 128);
    u8g2.drawStr(10, 63, "Iniciando...");
    u8g2.sendBuffer();
}

//   display_wifi_connecting                  
void display_wifi_connecting() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 12,  "WiFi");
    u8g2.drawStr(0, 26,  "Conectando...");
    u8g2.drawStr(0, 40,  WIFI_SSID);
    u8g2.sendBuffer();
}

//   display_wifi_connected                  ─
void display_wifi_connected(const String& ip) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 12, "WiFi OK");
    u8g2.drawStr(0, 26, ip.c_str());
    u8g2.sendBuffer();
    delay(1500);
}

//   display_wifi_failed                    
void display_wifi_failed() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 12, "WiFi FALLO");
    u8g2.drawStr(0, 26, "Modo offline");
    u8g2.sendBuffer();
    delay(1500);
}

//   display_aprsis_connecting                 
void display_aprsis_connecting() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 12, "APRS-IS");
    u8g2.drawStr(0, 26, "Conectando...");
    u8g2.drawStr(0, 40, APRSIS_SERVER);
    u8g2.sendBuffer();
}

//   display_aprsis_connected                 ─
void display_aprsis_connected() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 12, "APRS-IS OK");
    u8g2.drawStr(0, 26, "Login verificado");
    u8g2.sendBuffer();
    delay(1500);
}

//   display_aprsis_failed                   
void display_aprsis_failed() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 12, "APRS-IS FALLO");
    u8g2.drawStr(0, 26, "Reintentando...");
    u8g2.sendBuffer();
    delay(1500);
}

//   display_status  
void display_status(uint32_t rx_count, uint32_t tx_count) {
    char line[22];

    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_7x13B_tr);
    u8g2.drawStr(0, 12, SSID_APRS);

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 28, "W:OK  A:OK");

    snprintf(line, sizeof(line), "RX:%-4lu TX:%-4lu", rx_count, tx_count);
    u8g2.drawStr(0, 42, line);

    u8g2.drawHLine(0, 52, 128);
    u8g2.drawStr(0, 63, "Escuchando LoRa...");

    u8g2.sendBuffer();
}

//   display_packet  
void display_packet(const AprsFrame& frame) {
    char line[22];

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_7x13B_tr);
    u8g2.drawStr(0, 12, frame.src);

    u8g2.setFont(u8g2_font_6x10_tr);

    snprintf(line, sizeof(line), "RSSI: %d dBm", frame.rssi);
    u8g2.drawStr(0, 28, line);

    snprintf(line, sizeof(line), "SNR:  %.1f dB", frame.snr);
    u8g2.drawStr(0, 42, line);

    u8g2.drawHLine(0, 52, 128);
    u8g2.drawStr(0, 63, "Forwarded OK");

    u8g2.sendBuffer();
}

//   display_beacon  
void display_beacon() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 12, "Beacon TX");
    u8g2.drawStr(0, 26, SSID_APRS);
    u8g2.drawStr(0, 40, "433.775 MHz");
    u8g2.drawHLine(0, 52, 128);
    u8g2.drawStr(0, 63, "Transmitiendo...");
    u8g2.sendBuffer();
}