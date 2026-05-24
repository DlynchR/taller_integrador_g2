// wifi_manager.h — manejo de conexión WiFi
// Estado S1 de la FSM
#pragma once

#include <Arduino.h>

// Estados posibles de la conexión WiFi
enum class WifiState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    FAILED
};

// Intenta conectar al AP configurado en config.h
// Retorna true si la conexión fue exitosa
bool     wifi_connect();

// Retorna true si el WiFi sigue conectado
bool     wifi_is_connected();

// Llama esta función en el loop para detectar caídas
// y reconectar automáticamente
void     wifi_loop();

// Retorna el estado actual
WifiState wifi_get_state();

// Retorna la IP asignada como String (para el OLED)
String   wifi_get_ip();

// Retorna true si se agotaron los reintentos — FSM debe ir a S7
bool     wifi_failed_permanently();