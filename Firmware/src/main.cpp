// main.cpp — iGate APRS TI2TEC
// LILYGO T3 V1.6.1 — ESP32 + LoRa SX1276 + OLED SSD1306
//
// Máquina de estados:
//   S0: Boot
//   S1: WiFi connect
//   S2: APRS-IS connect
//   S3: Idle / escucha LoRa
//   S4: Decodificar paquete APRS
//   S5: Forward a APRS-IS
//   S6: Beacon TX
//   S7: Error / Watchdog
#include <Arduino.h>
#include <esp_task_wdt.h>
#include <math.h>

// ── Decodificadores de posición APRS ─────────────────────────
// Extraído de APRSPacketLib (CA2RXU - Ricardo Guzman)
// Soporta: texto simple, Base91 comprimido y Mic-E

static float _decodeBase91Lat(const String& s) {
    return 90.0 - (((s[0]-33)*pow(91,3) + (s[1]-33)*pow(91,2) + (s[2]-33)*91 + (s[3]-33)) / 380926.0);
}

static float _decodeBase91Lon(const String& s) {
    return -180.0 + (((s[0]-33)*pow(91,3) + (s[1]-33)*pow(91,2) + (s[2]-33)*91 + (s[3]-33)) / 190463.0);
}

static float _decodeLat(const String& s) {
    float v = s.substring(0,2).toFloat() + s.substring(2,4).toFloat()/60 + s.substring(5,7).toFloat()/6000;
    return s.endsWith("S") ? -v : v;
}

static float _decodeLon(const String& s) {
    float v = s.substring(0,3).toFloat() + s.substring(3,5).toFloat()/60 + s.substring(6,8).toFloat()/6000;
    return s.endsWith("W") ? -v : v;
}

static float _decodeMiceLat(const String& tocall) {
    String gpsLat; String ns = "S";
    for (int i = 0; i < 6; i++) {
        char c = tocall[i];
        gpsLat += (c > '9') ? char(int(c)-32) : c;
        if (i == 3) { gpsLat += "."; if (c > '9') ns = "N"; }
    }
    gpsLat += ns;
    return _decodeLat(gpsLat);
}

static float _decodeMiceLon(const String& tocall, const String& info) {
    bool offset = tocall[4] > '9';
    String we = tocall[5] > '9' ? "E" : "W";
    int d28 = (int)info[0] - 28; if (offset) d28 += 100;
    String lon = (d28 < 100 ? "0" : "") + String(d28);
    int m28 = (int)info[1] - 28; if (m28 >= 60) m28 -= 60;
    lon += (m28 < 10 ? "0" : "") + String(m28) + ".";
    int h28 = (int)info[2] - 28;
    lon += (h28 < 10 ? "0" : "") + String(h28) + we;
    return _decodeLon(lon);
}

// Extrae lat/lon de un frame APRS completo
// Retorna true si encontró posición válida
static bool aprs_extract_position(const String& packet, float& lat, float& lon) {
    lat = 0; lon = 0;

    // ── Mic-E (:` o :') ──────────────────────────────────────
    int miceIdx = packet.indexOf(":`");
    if (miceIdx < 0) miceIdx = packet.indexOf(":'");
    if (miceIdx > 10) {
        String tocall = packet.substring(packet.indexOf(">")+1, packet.indexOf(",") > 0 ? packet.indexOf(",") : packet.indexOf(":"));
        String info   = packet.substring(miceIdx + 2);
        lat = _decodeMiceLat(tocall);
        lon = _decodeMiceLon(tocall, info);
        return (lat != 0 || lon != 0);
    }

    // ── Posición con := :! :@ ─────────────────────────────────
    String gpsChars = "";
    int offset = 2;
    if (packet.indexOf(":=") > 10) gpsChars = ":=";
    else if (packet.indexOf(":!") > 10) gpsChars = ":!";
    else if (packet.indexOf(":@") > 10) { gpsChars = ":@"; offset = 9; }
    if (gpsChars == "") return false;

    int idx = packet.indexOf(gpsChars) + offset;

    // Detectar Base91 por el char en posición +12
    char det = packet[idx + 12];
    if (det=='G'||det=='Q'||det=='['||det=='H'||det=='X'||det=='T') {
        lat = _decodeBase91Lat(packet.substring(idx+1, idx+5));
        lon = _decodeBase91Lon(packet.substring(idx+5, idx+9));
    } else {
        lat = _decodeLat(packet.substring(idx,   idx+8));
        lon = _decodeLon(packet.substring(idx+9, idx+18));
    }
    return (lat != 0 || lon != 0);
}

#include "config.h"
#include "wifi_manager.h"
#include "lora_driver.h"
#include "aprs_parser.h"
#include "aprs_is.h"
#include "beacon.h"
#include "display.h"

//   Definición de estados  
enum class State {
    S0_BOOT,
    S1_WIFI_CONNECT,
    S2_APRSIS_CONNECT,
    S3_IDLE,
    S4_DECODE,
    S5_FORWARD,
    S6_BEACON_TX,
    S7_ERROR
};

//   Variables globales    
static State    current_state = State::S0_BOOT;
static uint32_t rx_count      = 0;
static uint32_t tx_count      = 0;
static uint8_t  error_count   = 0;
static uint32_t stations      = 0;      // estaciones únicas escuchadas

static uint8_t   rx_buf[220];
static uint8_t   rx_len       = 0;
static AprsFrame current_frame;

// Datos de la última estación escuchada (para el OLED)
static char    last_call[12]  = "";
static int16_t last_rssi      = 0;
static float   last_dist_km   = -1.0f;

static const uint8_t MAX_ERRORS = 5;

// ── Cálculo de distancia Haversine ───────────────────────────
// Calcula la distancia en km entre dos coordenadas GPS
static float haversine(float lat1, float lon1, float lat2, float lon2) {
    const float R = 6371.0f;  // radio de la Tierra en km
    float dlat = radians(lat2 - lat1);
    float dlon = radians(lon2 - lon1);
    float a = sin(dlat/2)*sin(dlat/2) +
              cos(radians(lat1))*cos(radians(lat2))*
              sin(dlon/2)*sin(dlon/2);
    float c = 2 * atan2(sqrt(a), sqrt(1-a));
    return R * c;
}

static void go_to(State next) {
    Serial.printf("[FSM] S%d → S%d\n", (int)current_state, (int)next);
    current_state = next;
}

// setup() — S0: Boot
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n[S0] Boot — " SSID_APRS " v" FW_VERSION);

    //   Watchdog       
    esp_task_wdt_init(WDT_TIMEOUT_S, true);
    esp_task_wdt_add(NULL);

    //   Display       
    display_init();

    //   LoRa         
    if (!lora_init()) {
        Serial.println("[S0] ERROR: LoRa no inicializó");
        display_aprsis_failed();
        go_to(State::S7_ERROR);
        return;
    }

    //   Beacon timer     
    beacon_init();

    // Avanzar a S1
    go_to(State::S1_WIFI_CONNECT);
}

 // loop() - ejecuta la FSM en cada ciclo
void loop() {
    // Alimentar watchdog en cada iteración
    esp_task_wdt_reset();

    switch (current_state) {

    // S1: WiFi connect
    case State::S1_WIFI_CONNECT: {
        display_wifi_connecting();
        bool ok = wifi_connect();
        if (ok) {
            display_wifi_connected(wifi_get_ip());
            go_to(State::S2_APRSIS_CONNECT);
        } else {
            display_wifi_failed();
            lora_receive_mode();
            go_to(State::S3_IDLE);
        }
        break;
    }

    // S2: APRS-IS connect            
    case State::S2_APRSIS_CONNECT: {
        display_aprsis_connecting();
        bool ok = aprsis_connect();
        if (ok) {
            display_aprsis_connected();
            error_count = 0;
            lora_receive_mode();
            go_to(State::S3_IDLE);
        } else {
            display_aprsis_failed();
            error_count++;
            if (error_count >= MAX_ERRORS) {
                go_to(State::S7_ERROR);
            } else {
                // Esperar 10 s y reintentar
                delay(10000);
                go_to(State::S2_APRSIS_CONNECT);
            }
        }
        break;
    }

    // S3: Idle / escucha LoRa            
    case State::S3_IDLE: {
        // Actualizar pantalla principal con todos los datos
        display_status(rx_count, tx_count,
                       last_call, last_rssi,
                       last_dist_km, stations);

        //   Verificar conexión APRS-IS             
        if (wifi_is_connected() &&
            aprsis_get_state() == AprsIsState::DISCONNECTED) {
            Serial.println("[S3] APRS-IS caído — reconectando");
            go_to(State::S2_APRSIS_CONNECT);
            break;
        }

        //   Verificar reconexión WiFi (modo offline)      
        if (!wifi_is_connected()) {
            wifi_loop();
            // Si se agotaron los reintentos → reiniciar el sistema
            if (wifi_failed_permanently()) {
                Serial.println("[S3] WiFi falló permanentemente → S7");
                go_to(State::S7_ERROR);
                break;
            }
        }

        //   Paquete LoRa recibido → S4             
        if (lora_packet_available()) {
            rx_len = lora_read_packet(rx_buf, 220);
            if (rx_len > 0) {
                go_to(State::S4_DECODE);
                break;
            }
        }

        //   Timer beacon vencido → S6              
        if (beacon_is_due()) {
            go_to(State::S6_BEACON_TX);
            break;
        }

        // Sin eventos — pequeño delay para no saturar el CPU
        delay(10);
        break;
    }

    // S4: Decodificar paquete APRS
    case State::S4_DECODE: {
        bool ok = aprs_parse(rx_buf, rx_len,
                             lora_get_rssi(), lora_get_snr(),
                             current_frame);
        if (ok) {
            rx_count++;
            stations++;

            // Guardar datos para el OLED
            strncpy(last_call, current_frame.src, sizeof(last_call)-1);
            last_rssi = current_frame.rssi;

            // Extraer lat/lon usando decodificadores embebidos
            // soporta texto simple, Base91 y Mic-E
            float plat, plon;
            String rawStr = String(current_frame.raw);
            if (aprs_extract_position(rawStr, plat, plon)) {
                last_dist_km = haversine(BEACON_LAT, BEACON_LON, plat, plon);
                if (last_dist_km > 2000.0f) last_dist_km = -1.0f;
                else Serial.printf("[FSM] Distancia: %.1f km\n", last_dist_km);
            } else {
                last_dist_km = -1.0f;
            }

            display_packet(current_frame);
            go_to(State::S5_FORWARD);
        } else {
            Serial.println("[S4] Frame inválido — descartado");
            go_to(State::S3_IDLE);
        }
        break;
    }

    // S5: Forward a APRS-IS
    case State::S5_FORWARD: {
        if (!aprsis_is_connected()) {
            Serial.println("[S5] Sin conexión APRS-IS — descartando");
            go_to(State::S3_IDLE);
            break;
        }

        String formatted = aprs_format_for_igate(current_frame);
        bool ok = aprsis_send(formatted);

        if (ok) {
            tx_count++;
            Serial.printf("[S5] Forward OK — RX:%lu TX:%lu\n", rx_count, tx_count);
        } else {
            error_count++;
            if (error_count >= MAX_ERRORS) {
                go_to(State::S7_ERROR);
                break;
            }
        }

        go_to(State::S3_IDLE);
        break;
    }

    // S6: Beacon TX
    case State::S6_BEACON_TX: {
        display_beacon();
        bool ok = beacon_send();
        if (!ok) {
            error_count++;
            if (error_count >= MAX_ERRORS) {
                go_to(State::S7_ERROR);
                break;
            }
        }

        go_to(State::S3_IDLE);
        break;
    }

    // S7: Error / Watchdog
    case State::S7_ERROR: {
        Serial.println("[S7] Error crítico — reiniciando en 3s...");
        display_aprsis_failed();
        delay(3000);
        ESP.restart();
        break;
    }

    } // fin switch
}