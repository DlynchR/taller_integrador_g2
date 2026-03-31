
// wifi_manager.cpp — manejo de conexión WiFi
// Estado S1 de la FSM
#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>
#include <esp_task_wdt.h>

//  Variables internas 
static WifiState _state          = WifiState::DISCONNECTED; // _state  : estado actual de la conexión WiFi
static uint32_t  _last_attempt   = 0;   // _last_attempt : marca de tiempo del último intento de conexión
static uint8_t   _retry_count    = 0;   // _retry_count  : cantidad de reintentos fallidos acumulados
static const uint8_t  MAX_RETRIES       = 5;    // Máximo de reintentos antes de declarar fallo permanente
static const uint32_t RETRY_INTERVAL_MS = 30000;    // Tiempo de espera entre reintentos (30 segundos)

// Intenta conectar al AP definido en config.h
// Bloquea el loop hasta conectar o vencer el timeout
// Retorna true si la conexión fue exitosa
bool wifi_connect() {
    Serial.printf("[WiFi] Conectando a %s ...\n", WIFI_SSID);
    _state = WifiState::CONNECTING;

    // Modo estación (cliente) — no levanta punto de acceso propio
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        // Si pasa el tiempo límite sin conectar → fallo
        if (millis() - start >= WIFI_TIMEOUT_MS) {
            Serial.println("[WiFi] Timeout — sin conexión");
            _state = WifiState::FAILED;
            _retry_count++;
            return false;
        }
        // Alimentar el watchdog para evitar reset durante la espera
        esp_task_wdt_reset();
        delay(500);
        Serial.print(".");
    }

    // Conexión exitosa guardar estado y resetear contador
    _state       = WifiState::CONNECTED;
    _retry_count = 0;
    Serial.printf("\n[WiFi] Conectado. IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
}

// Consulta directamente el estado del driver WiFi del ESP32
// Más confiable que revisar _state porque detecta caídas externas
bool wifi_is_connected() {
    return WiFi.status() == WL_CONNECTED;
}

// Llamar desde el loop principal (S3) en cada ciclo
// Si el WiFi se cayó, espera RETRY_INTERVAL_MS y reintenta
// Si supera MAX_RETRIES, marca como FAILED y deja de intentar
void wifi_loop() {
    // Si sigue conectado no hay nada que hacer
    if (WiFi.status() == WL_CONNECTED) {
        _state = WifiState::CONNECTED;
        return;
    }

    // Conexión caída marcar y verificar si es momento de reintentar
    _state = WifiState::DISCONNECTED;

    // Backoff: esperar el intervalo antes de cada reintento
    if (millis() - _last_attempt < RETRY_INTERVAL_MS) return;
    _last_attempt = millis();

    // Si ya se agotaron los reintentos fallo permanente hasta reset
    if (_retry_count >= MAX_RETRIES) {
        Serial.println("[WiFi] Máximo de reintentos alcanzado");
        _state = WifiState::FAILED;
        return;
    }

    Serial.printf("[WiFi] Reintentando conexión (%d/%d)...\n", _retry_count + 1, MAX_RETRIES);
    wifi_connect();
}


// Retorna el estado interno — usado por la FSM para decisiones
WifiState wifi_get_state() { return _state; }


// Retorna la IP asignada como String para mostrar en el OLED
// Retorna "0.0.0.0" si no hay conexión activa
String wifi_get_ip() {
    if (!wifi_is_connected()) return "0.0.0.0";
    return WiFi.localIP().toString();
}