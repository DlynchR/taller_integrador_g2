
// beacon.h — Construcción y transmisión del beacon de posición
// Estado S6 de la FSM

#pragma once

#include <Arduino.h>

// Inicializa el timer del beacon
void   beacon_init();

// Retorna true si es momento de transmitir el beacon
bool   beacon_is_due();

// Construye y transmite el frame APRS de posición por LoRa
// Retorna true si el TX fue exitoso
bool   beacon_send();

// Retorna el frame APRS del último beacon como String
String beacon_get_last_frame();