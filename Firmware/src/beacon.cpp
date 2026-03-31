// beacon.cpp — Construcción y transmisión del beacon de posición
// Estado S6 de la FSM
//
// Formato APRS de posición sin timestamp:
//   TI1TEC-10>APLT00,WIDE2-1:!DDMM.mmN/DDDMM.mmW&comentario
//
// Ejemplo real:
//   TI1TEC-10>APLT00,WIDE2-1:!1001.33N/08403.36W&LoRa iGate CR

#include "beacon.h"
#include "config.h"
#include "lora_driver.h"
#include "aprs_is.h"

// Variables internas
static uint32_t _last_beacon   = 0;
static String   _last_frame    = "";

// Conversión grados decimales → formato APRS 
// APRS usa DDMM.mm (grados y minutos decimales), no grados decimales
// Ej: 10.0222° → "1001.33N"

static String _format_lat(float lat) {
    char buf[12];
    char hemi = (lat >= 0) ? 'N' : 'S';
    lat = fabs(lat);
    int  deg  = (int)lat;
    float min = (lat - deg) * 60.0f;
    snprintf(buf, sizeof(buf), "%02d%05.2f%c", deg, min, hemi);
    return String(buf);
}

// Ej: -84.0560° → "08403.36W"
static String _format_lon(float lon) {
    char buf[12];
    char hemi = (lon >= 0) ? 'E' : 'W';
    lon = fabs(lon);
    int  deg  = (int)lon;
    float min = (lon - deg) * 60.0f;
    snprintf(buf, sizeof(buf), "%03d%05.2f%c", deg, min, hemi);
    return String(buf);
}

// beacon_init 
void beacon_init() {
    // Arrancar con timer en 0 para que el primer beacon
    // se envíe al inicio sin esperar el intervalo completo
    _last_beacon = 0;
    Serial.printf("[Beacon] Intervalo: %d min\n",
                  BEACON_INTERVAL / 60000);
}

// beacon_is_due 
bool beacon_is_due() {
    return (millis() - _last_beacon) >= BEACON_INTERVAL;
}

// beacon_send                
bool beacon_send() {
    // Construir frame APRS de posición        
    // Formato: SRC>DST,PATH:!LAT/LONsym comentario
    String lat_str = _format_lat(BEACON_LAT);   // "1001.33N"
    String lon_str = _format_lon(BEACON_LON);   // "08403.36W"

    // Frame completo
    // '!' = posición sin timestamp
    // '/' = tabla de símbolos primaria
    // '&' = símbolo iGate
    String frame = String(SSID_APRS) + ">APLT00,WIDE2-1" +
                   ":!" +
                   lat_str + "/" +
                   lon_str + "&" +
                   APRS_COMMENT;

    _last_frame = frame;

    Serial.println("[Beacon] Frame: " + frame);

    // Transmitir por LoRa            
    uint8_t buf[256];
    uint8_t len = frame.length();

    if (len > sizeof(buf) - 1) {
        Serial.println("[Beacon] Frame demasiado largo");
        return false;
    }

    memcpy(buf, frame.c_str(), len);

    bool ok = lora_transmit(buf, len);

    if (ok) {
        _last_beacon = millis();
        Serial.println("[Beacon] TX OK");

        // También subir la posición a APRS-IS por TCP
        // así aparece en aprs.fi aunque no haya otro iGate cerca
        if (aprsis_is_connected()) {
            String tcp_frame = frame + "\r\n";
            aprsis_send(tcp_frame);
            Serial.println("[Beacon] Posición subida a APRS-IS");
        }
    } else {
        Serial.println("[Beacon] TX fallido");
    }

    return ok;
}

// beacon_get_last_frame      
String beacon_get_last_frame() {
    return _last_frame;
}