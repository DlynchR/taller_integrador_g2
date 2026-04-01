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

static uint8_t  rx_buf[220];     // max frame APRS
static uint8_t  rx_len        = 0;
static AprsFrame current_frame;

static const uint8_t MAX_ERRORS = 5;   // errores antes de reset

//   Helpers         
static void go_to(State next) {
    Serial.printf("[FSM] S%d → S%d\n",
                  (int)current_state, (int)next);
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
        display_aprsis_failed();   // reutilizamos pantalla error
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
            Serial.println("[S1] Sin WiFi — modo offline");
            // Sin WiFi solo escuchamos LoRa, no podemos hacer forward
            // Reintentamos cada 30 s desde S3
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
        display_status(rx_count, tx_count);

        //   Verificar conexión APRS-IS             
        if (wifi_is_connected() &&
            aprsis_get_state() == AprsIsState::DISCONNECTED) {
            Serial.println("[S3] APRS-IS caído — reconectando");
            go_to(State::S2_APRSIS_CONNECT);
            break;
        }

        //   Verificar reconexión WiFi (modo offline)      
        if (!wifi_is_connected()) {
            wifi_loop();   // intenta reconectar con backoff
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
            Serial.printf("[S5] Forward OK — RX:%lu TX:%lu\n",
                          rx_count, tx_count);
        } else {
            Serial.println("[S5] Error forward — TCP caído");
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

        if (ok) {
            Serial.println("[S6] Beacon TX OK");
        } else {
            Serial.println("[S6] Beacon TX fallido");
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