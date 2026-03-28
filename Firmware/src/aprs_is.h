// aprs_is.h — conexión TCP con servidor APRS-IS
// Estados S2 (connect) y S5 (forward) de la FSM
#pragma once

#include <Arduino.h>

// Estados de la conexión APRS-IS
enum class AprsIsState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    FAILED
};

// Abre la conexión TCP y realiza el login con callsign/passcode
// Retorna true si el servidor responde "verified"
bool        aprsis_connect();

// Retorna true si la conexión TCP sigue activa
bool        aprsis_is_connected();

// Envía un frame ya formateado al servidor APRS-IS
// Retorna true si el envío fue exitoso
bool        aprsis_send(const String& frame);

// Cierra la conexión TCP limpiamente
void        aprsis_disconnect();

// Retorna el estado actual
AprsIsState aprsis_get_state();