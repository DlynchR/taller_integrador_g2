// ============================================================
// aprs_parser.h — decodificación de frames APRS sobre LoRa
// Usado en estado S4 de la FSM
// ============================================================
#pragma once

#include <Arduino.h>

// Estructura con los campos extraídos de un frame APRS
struct AprsFrame {
    char    src[12];        // callsign origen  ej: "TI2TEC-9"
    char    dst[12];        // callsign destino ej: "APRS"
    char    path[64];       // path de digipeaters ej: "WIDE1-1,WIDE2-1"
    char    payload[220];   // información APRS (posición, telemetría, etc.)
    char    raw[256];       // frame completo como string
    int16_t rssi;           // RSSI al momento de recepción
    float   snr;            // SNR al momento de recepción
    bool    valid;          // true si el frame fue parseado correctamente
};

// Parsea un buffer de bytes recibido por LoRa
// Llena la estructura AprsFrame y retorna true si es válido
bool aprs_parse(const uint8_t* buf, uint8_t len,
                int16_t rssi, float snr,
                AprsFrame& frame);

// Retorna el frame formateado para subir a APRS-IS
// Formato: CALLSIGN>DST,PATH,qAR,IGATE:payload
String aprs_format_for_igate(const AprsFrame& frame);