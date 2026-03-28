// aprs_is.cpp — conexión TCP con servidor APRS-IS
// Estados S2 (connect) y S5 (forward) de la FSM
#include "aprs_is.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_task_wdt.h>

//  Variables internas 
static WiFiClient   _client;
static AprsIsState  _state          = AprsIsState::DISCONNECTED;
static uint32_t     _last_keepalive = 0;

static const uint32_t CONNECT_TIMEOUT_MS  = 10000;   // 10 s para conectar
static const uint32_t KEEPALIVE_INTERVAL  = 120000;  // 2 min — comentario vacío
static const uint32_t READ_TIMEOUT_MS     = 5000;    // 5 s esperando respuesta login

//  aprsis_connect 
bool aprsis_connect() {
    if (_client.connected()) {
        _client.stop();
    }

    Serial.printf("[APRS-IS] Conectando a %s:%d ...\n",
                  APRSIS_SERVER, APRSIS_PORT);
    _state = AprsIsState::CONNECTING;

    //  Paso 1: abrir socket TCP 
    if (!_client.connect(APRSIS_SERVER, APRSIS_PORT)) {
        Serial.println("[APRS-IS] Error TCP — no se pudo conectar");
        _state = AprsIsState::FAILED;
        return false;
    }
    _client.setTimeout(READ_TIMEOUT_MS);

    //  Paso 2: leer banner del servidor 
    uint32_t start = millis();
    while (!_client.available()) {
        if (millis() - start > CONNECT_TIMEOUT_MS) {
            Serial.println("[APRS-IS] Timeout esperando banner");
            _client.stop();
            _state = AprsIsState::FAILED;
            return false;
        }
        esp_task_wdt_reset();   // evitar watchdog durante espera
        delay(100);
    }
    String banner = _client.readStringUntil('\n');
    Serial.println("[APRS-IS] Banner: " + banner);

    // Paso 3: enviar string de login 
    // Formato: user CALLSIGN pass PASSCODE vers FIRMWARE VER
    String login = "user " + String(CALLSIGN) +
                   " pass " + String(PASSCODE) +
                   " vers TI1TEC-iGate " + String(FW_VERSION) +
                   " filter " + String(APRSIS_FILTER) + "\r\n";

    _client.print(login);
    Serial.print("[APRS-IS] Login enviado: " + login);

    //  Paso 4: verificar respuesta del servidor 
    start = millis();
    while (!_client.available()) {
        if (millis() - start > READ_TIMEOUT_MS) {
            Serial.println("[APRS-IS] Timeout esperando respuesta login");
            _client.stop();
            _state = AprsIsState::FAILED;
            return false;
        }
        esp_task_wdt_reset();   // evitar watchdog durante espera
        delay(100);
    }

    String response = _client.readStringUntil('\n');
    Serial.println("[APRS-IS] Respuesta: " + response);

    // La respuesta debe contener "verified" para confirmar login
    if (response.indexOf("verified") < 0) {
        Serial.println("[APRS-IS] Login no verificado — revisa tu passcode");
        _client.stop();
        _state = AprsIsState::FAILED;
        return false;
    }

    _state          = AprsIsState::CONNECTED;
    _last_keepalive = millis();
    Serial.println("[APRS-IS] Conectado y verificado");
    return true;
}

// aprsis_is_connected 
bool aprsis_is_connected() {
    return _client.connected();
}

//  aprsis_send 
bool aprsis_send(const String& frame) {
    if (!_client.connected()) {
        Serial.println("[APRS-IS] No conectado — no se puede enviar");
        _state = AprsIsState::DISCONNECTED;
        return false;
    }

    size_t sent = _client.print(frame);
    if (sent == 0) {
        Serial.println("[APRS-IS] Error al enviar frame");
        _state = AprsIsState::DISCONNECTED;
        return false;
    }

    // Actualizar keepalive
    _last_keepalive = millis();

    Serial.print("[APRS-IS] Enviado: " + frame);
    return true;
}

//  aprsis_disconnect 
void aprsis_disconnect() {
    if (_client.connected()) {
        _client.stop();
    }
    _state = AprsIsState::DISCONNECTED;
    Serial.println("[APRS-IS] Desconectado");
}

//  aprsis_get_state 
AprsIsState aprsis_get_state() {
    // Detectar caída silenciosa de TCP
    if (_state == AprsIsState::CONNECTED && !_client.connected()) {
        Serial.println("[APRS-IS] Conexión caída detectada");
        _state = AprsIsState::DISCONNECTED;
    }

    // Keepalive — enviar comentario vacío para mantener viva la conexión
    if (_state == AprsIsState::CONNECTED &&
        millis() - _last_keepalive > KEEPALIVE_INTERVAL) {
        _client.print("# keepalive\r\n");
        _last_keepalive = millis();
        Serial.println("[APRS-IS] Keepalive enviado");
    }

    return _state;
}