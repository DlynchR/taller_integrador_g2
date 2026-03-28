// ============================================================
// aprs_parser.cpp — decodificación de frames APRS sobre LoRa
// Usado en estado S4 de la FSM
//
// Formato estándar de un frame APRS sobre LoRa:
//   SRC>DST,PATH:payload\0
//   ej: TI2TEC-9>APLT23,WIDE1-1:!1001.33N/08403.36W>
// ============================================================
#include "aprs_parser.h"
#include "config.h"

// ── aprs_parse ───────────────────────────────────────────────
bool aprs_parse(const uint8_t* buf, uint8_t len,
                int16_t rssi, float snr,
                AprsFrame& frame) {

    // Limpiar estructura
    memset(&frame, 0, sizeof(AprsFrame));
    frame.rssi  = rssi;
    frame.snr   = snr;
    frame.valid = false;

    // Validación básica de longitud
    if (len < 10 || len > 255) {
        Serial.println("[Parser] Frame demasiado corto o largo");
        return false;
    }

    // Copiar buffer como string (asegurar terminador nulo)
    memcpy(frame.raw, buf, len);
    frame.raw[len] = '\0';

    String raw = String(frame.raw);

    // ── Buscar separador '>' entre src y dst ─────────────────
    int gt_pos = raw.indexOf('>');
    if (gt_pos < 1 || gt_pos > 10) {
        Serial.println("[Parser] No se encontró '>'");
        return false;
    }

    String src = raw.substring(0, gt_pos);
    src.trim();
    if (src.length() < 3 || src.length() > 9) {
        Serial.println("[Parser] Callsign origen inválido");
        return false;
    }
    src.toCharArray(frame.src, sizeof(frame.src));

    // ── Buscar separador ':' entre header y payload ───────────
    int colon_pos = raw.indexOf(':');
    if (colon_pos < 0) {
        Serial.println("[Parser] No se encontró ':'");
        return false;
    }

    // Header = todo entre '>' y ':'
    String header = raw.substring(gt_pos + 1, colon_pos);

    // ── Separar dst y path dentro del header ─────────────────
    int comma_pos = header.indexOf(',');
    String dst, path;

    if (comma_pos < 0) {
        // Sin path — solo destino
        dst  = header;
        path = "";
    } else {
        dst  = header.substring(0, comma_pos);
        path = header.substring(comma_pos + 1);
    }

    dst.trim();
    path.trim();
    dst.toCharArray(frame.dst,   sizeof(frame.dst));
    path.toCharArray(frame.path, sizeof(frame.path));

    // ── Extraer payload ───────────────────────────────────────
    String payload = raw.substring(colon_pos + 1);
    payload.trim();

    if (payload.length() == 0) {
        Serial.println("[Parser] Payload vacío");
        return false;
    }
    payload.toCharArray(frame.payload, sizeof(frame.payload));

    // ── Frame válido ──────────────────────────────────────────
    frame.valid = true;

    Serial.printf("[Parser] OK  src=%-9s dst=%-6s rssi=%d snr=%.1f\n",
                  frame.src, frame.dst, frame.rssi, frame.snr);

    return true;
}

// ── aprs_format_for_igate ────────────────────────────────────
// Construye el string que se envía por TCP a APRS-IS
// Formato estándar de iGate:
//   SRC>DST,PATH,qAR,IGATE-SSID:payload\r\n
String aprs_format_for_igate(const AprsFrame& frame) {
    String out = String(frame.src) + ">" + String(frame.dst);

    if (strlen(frame.path) > 0) {
        out += "," + String(frame.path);
    }

    out += ",qAR," + String(SSID_APRS);
    out += ":" + String(frame.payload);
    out += "\r\n";

    return out;
}