
// lora_driver.cpp — Comunicación con módulo LoRa SX1276
// Usado en estados S3 (Escucha), S4 (Recepción), S6 (Beacon TX)

#include "lora_driver.h"
#include "config.h"

#include <SPI.h>
#include <RadioLib.h>

// Instancia RadioLib 
// SX1276(CS, DIO0, RST, DIO1)
static SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1);

// Variables internas 
static volatile bool _packet_flag  = false;   // flag de interrupción
static uint8_t       _rx_buf[256]  = {0};
static uint8_t       _rx_len       = 0;
static int16_t       _last_rssi    = 0;
static float         _last_snr     = 0.0f;

// ISR — Se dispara cuando DIO0 indica paquete recibido 
void IRAM_ATTR lora_isr() {
    _packet_flag = true;
}

// lora_init
bool lora_init() {
    // Configurar bus SPI con los pines del LILYGO T3 V1.6.1
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    Serial.println("[LoRa] Inicializando SX1276...");

    int16_t state = radio.begin(
        LORA_FREQ / 1e6,    // frecuencia en MHz
        LORA_BW  / 1e3,     // ancho de banda en kHz
        LORA_SF,            // spreading factor
        LORA_CR,            // coding rate
        LORA_SYNC,          // sync word
        LORA_POWER,         // potencia en dBm
        LORA_PREAMBLE       // longitud preámbulo
    );

    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Error init: %d\n", state);
        return false;
    }

    // Fijar CRC — Requerido por el estándar LoRa APRS
    radio.setCRC(false);

    // Adjuntar ISR al pin DIO0
    radio.setDio0Action(lora_isr, RISING);

    Serial.printf("[LoRa] OK — %.3f MHz  SF%d  BW%.0fkHz\n",
                  LORA_FREQ / 1e6, LORA_SF, LORA_BW / 1e3);
    return true;
}

// lora_receive_mode 
void lora_receive_mode() {
    _packet_flag = false;
    radio.startReceive();
}

// lora_packet_available 
bool lora_packet_available() {
    return _packet_flag;
}

// lora_read_packet 
uint8_t lora_read_packet(uint8_t* buf, uint8_t max_len) {
    if (!_packet_flag) return 0;
    _packet_flag = false;

    _rx_len = 0;
    int16_t state = radio.readData(_rx_buf, sizeof(_rx_buf));

    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Error lectura: %d\n", state);
        lora_receive_mode();
        return 0;
    }

    _last_rssi = radio.getRSSI();
    _last_snr  = radio.getSNR();
    _rx_len    = radio.getPacketLength();

    uint8_t copy_len = (_rx_len < max_len) ? _rx_len : max_len;
    memcpy(buf, _rx_buf, copy_len);

    Serial.printf("[LoRa] Paquete %d bytes | RSSI: %d dBm | SNR: %.1f dB\n",
                  copy_len, _last_rssi, _last_snr);

    // Volver a modo recepción inmediatamente
    lora_receive_mode();
    return copy_len;
}

// lora_get_rssi 
int16_t lora_get_rssi() {
    return _last_rssi;
}

// lora_get_snr 
float lora_get_snr() {
    return _last_snr;
}

// lora_transmit 
bool lora_transmit(const uint8_t* buf, uint8_t len) {
    Serial.printf("[LoRa] Transmitiendo %d bytes...\n", len);

    // Detener recepción antes de transmitir
    radio.standby();

    int16_t state = radio.transmit((const char*)buf, len);

    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LoRa] Error TX: %d\n", state);
        lora_receive_mode();
        return false;
    }

    Serial.println("[LoRa] TX OK");
    lora_receive_mode();
    return true;
}