// lora_driver.h — Comunicación con módulo LoRa SX1276
// Usado en estados S3 (escucha), S4 (recepción), S6 (beacon TX)

#pragma once

#include <Arduino.h>

// Inicializa el módulo LoRa con los parámetros de config.h
// Retorna true si el SX1276 responde correctamente por SPI
bool   lora_init();

// Pone el módulo en modo recepción continua
// Llamar después de cada TX o al entrar a S3
void   lora_receive_mode();

// Retorna true si hay un paquete nuevo disponible para leer
bool   lora_packet_available();

// Copia el último paquete recibido al buffer proporcionado
// Retorna el número de bytes copiados, 0 si no hay paquete
uint8_t lora_read_packet(uint8_t* buf, uint8_t max_len);

// Retorna el RSSI del último paquete recibido (dBm)
int16_t lora_get_rssi();

// Retorna el SNR del último paquete recibido (dB)
float   lora_get_snr();

// Transmite un buffer de bytes
// Bloquea hasta que el TX termina o vence el timeout
// Retorna true si el envío fue exitoso
bool    lora_transmit(const uint8_t* buf, uint8_t len);