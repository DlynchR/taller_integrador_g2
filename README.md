# Taller_Integrador
El presente respositorio corresponde al trabajo realizado por el grupo 2, conformado por Alvaro Chacón y Denzel Lynch, en el curso EL5610 Taller Integrador de la carrera de Ingeniería en Electrónica del Instituto Tecnológico de Costa Rica.

En el siguiente enlace podrá observar generalidades del protocolo APRS y LORA:

https://www.canva.com/design/DAHCYHS7kGw/CJDRDF5Uz_gmbKo0bwzgLw/edit?utm_content=DAHCYHS7kGw&utm_campaign=designshare&utm_medium=link2&utm_source=sharebutton

## 📅 Cronograma del Proyecto — Firmware iGate APRS (Grupo 2)
```mermaid
gantt
    title Firmware iGate APRS — Grupo 2
    dateFormat  YYYY-MM-DD
    axisFormat  %d %b

    section Investigación
    Investigación LoRa y APRS              :a1, 2025-02-23, 14d
    Investigación legislación PNAF         :a2, 2025-02-23, 14d

    section Configuración
    Configuración entorno y repositorio    :b1, 2025-02-23, 14d
    Definición arquitectura del iGate      :b2, 2025-03-02, 14d
    Diagramas de bloques y estados         :b3, 2025-03-02, 21d
    Selección hardware      :b4, 2025-03-09, 14d

    section Diseño
    Diseño de tramas APRS                  :c1, 2025-03-09, 21d

    section Desarrollo
    Desarrollo firmware base               :d1, 2025-03-16, 21d
    Comunicación LoRa (recepción)          :d2, 2025-03-23, 21d
    Conexión a APRS-IS (Internet)          :d3, 2025-03-30, 21d

    section Pruebas
    Pruebas de transmisión a APRS          :e1, 2025-04-06, 21d
    Integración con trackers               :e2, 2025-04-13, 21d
    Depuración y optimización              :e3, 2025-04-20, 28d

    section Publicación y Cierre
    Publicación en APRS.fi                 :f1, 2025-04-20, 42d
    Documentación final                    :f2, 2025-05-18, 28d
    Preparación presentación final         :f3, 2025-05-25, 21d
```


## Máquina de Estados — iGate APRS ESP32 LILYGO

### Diagrama de transiciones

| Estado actual | Evento / Condición | Estado siguiente |
|---|---|---|
| S0: Boot | Siempre | S1: WiFi Connect |
| S1: WiFi Connect | Conexión exitosa | S2: APRS-IS Connect |
| S1: WiFi Connect | Timeout / fallo | Sin WiFi (modo offline) |
| Sin WiFi | WiFi disponible (retry) | S1: WiFi Connect |
| S2: APRS-IS Connect | Login OK | S3: Idle / Escucha |
| S2: APRS-IS Connect | Error TCP | S7: Error / Watchdog |
| S3: Idle / Escucha | Paquete LoRa recibido | S4: Decodificar APRS |
| S3: Idle / Escucha | Timer beacon vencido | S6: Beacon TX |
| S3: Idle / Escucha | Conexión TCP caída | S2: APRS-IS Connect |
| S4: Decodificar APRS | Frame válido | S5: Forward APRS-IS |
| S4: Decodificar APRS | Frame inválido | S3: Idle / Escucha (descarta) |
| S5: Forward APRS-IS | Enviado correctamente | S3: Idle / Escucha |
| S5: Forward APRS-IS | Error TCP | S7: Error / Watchdog |
| S6: Beacon TX | TX completado | S3: Idle / Escucha |
| S6: Beacon TX | Error LoRa | S7: Error / Watchdog |
| S7: Error / Watchdog | Fallo recuperable | S1: WiFi Connect |
| S7: Error / Watchdog | Fallo crítico | S0: Boot (ESP.restart()) |

---
![FSM iGate APRS ESP32](fsm_igate_aprs_esp32.svg)
### Descripción de cada estado


### S0: Boot
**Color:** Gris (arranque)

Primer estado al encender o reiniciar el ESP32. Inicializa todos los periféricos de hardware
No tiene condición de salida: siempre avanza a S1 al terminar la inicialización. Si algún periférico falla (por ejemplo el LoRa no responde en SPI), se puede redirigir directamente a S7.

---

### S1: WiFi Connect
**Color:** Azul (conexión de red)

Intenta conectarse al punto de acceso WiFi configurado usando las credenciales almacenadas (SSID + password en `config.h` o EEPROM). Muestra en el OLED el estado de conexión y la dirección IP asignada al conectarse.

- Si la conexión es exitosa → avanza a S2.
- Si vence el timeout (típicamente 20–30 segundos) → entra al modo Sin WiFi para operar de forma limitada.

---

### Sin WiFi (modo offline)
**Color:** Ámbar (advertencia)

Estado de contingencia cuando no hay red disponible. El iGate sigue funcionando parcialmente:
- El receptor LoRa permanece activo y puede escuchar paquetes.
- Los paquetes recibidos se muestran en el OLED pero no se suben a APRS-IS.
- Se intenta reconectar al WiFi periódicamente.

Cuando el WiFi vuelve a estar disponible, retorna a S1 para restablecer la conexión completa.

---

### S2: APRS-IS Connect
**Color:** Azul (conexión de red)

Establece la conexión TCP con el servidor de la red APRS-IS. El flujo es:
1. Resolver DNS de `rotate.aprs2.net` (o servidor regional).
2. Abrir socket TCP en el puerto 14580.
3. Enviar el string de login: `user CALLSIGN pass PASSCODE vers firmware_version`.
4. Verificar la respuesta del servidor (`# logresp ... verified`).

Si el login es verificado → avanza a S3. Si falla (servidor no responde, credenciales incorrectas, error TCP) → va a S7.

---

### S3: Idle / Escucha LoRa
**Color:** Teal (espera activa)

Estado central donde el iGate pasa la mayor parte del tiempo. El módulo LoRa está en modo recepción continua (`receiveMode()`). En cada ciclo del loop se verifican tres condiciones en paralelo:

1. **¿Llegó un paquete LoRa?** → S4
2. **¿Venció el timer del beacon?** → S6
3. **¿Se cayó la conexión TCP?** → S2 para reconectar

También actualiza el OLED periódicamente con la hora, cantidad de paquetes recibidos y estado de la conexión.

---

### S4: Decodificar APRS
**Color:** Púrpura (proceso APRS)

Recibe el buffer de bytes del módulo LoRa y lo procesa para extraer el frame APRS. Las tareas son:
- Verificar el preámbulo y el CRC del paquete LoRa.
- Decodificar el protocolo AX.25 (dirección origen, destino, path de digipeaters).
- Parsear el payload APRS: tipo de paquete (posición, telemetría, mensaje, objeto).
- Mostrar en el OLED: callsign, RSSI y SNR de la señal recibida.

Si el frame es válido y bien formado → S5. Si está corrupto o no es APRS válido → descarta y vuelve a S3 sin subir nada.

---

### S5: Forward APRS-IS
**Color:** Púrpura (proceso APRS)

Toma el frame APRS ya decodificado y lo reenvía al servidor APRS-IS a través de la conexión TCP establecida en S2. El paquete se envía con el formato estándar de iGate:

```
CALLSIGN>APRS,TCPIP*,qAR,IGATE-CALLSIGN:frame_original
```

Después del envío actualiza el contador de paquetes forwarded en el OLED. Si el TCP falla durante el envío → S7. Si el envío es exitoso → regresa a S3 a seguir escuchando.

---

### S6: Beacon TX
**Color:** Coral (transmisión)

Transmite la posición GPS del iGate como un paquete APRS por LoRa, para que otros equipos en el aire (globos, trackers) puedan ver la ubicación de la estación. El flujo es:
1. Leer coordenadas fijas desde la configuración (config.h).
2. Construir el frame APRS de posición con símbolo de iGate (`/&`).
3. Cambiar el módulo LoRa a modo transmisión (`transmit()`).
4. Enviar el paquete y esperar confirmación de TX completo.
5. Volver al modo recepción y regresar a S3.

El timer del beacon se reinicia al completar la transmisión.

---

### S7: Error / Watchdog
**Color:** Rojo (error)

Estado de manejo de fallos. Se activa ante cualquier error crítico: fallo de hardware, timeout de red, excepción no manejada, o el watchdog timer del ESP32 por un loop bloqueado.

Dependiendo de la gravedad:
- **Fallo recuperable** (WiFi caído, servidor APRS-IS no responde): espera unos segundos y regresa a S1 para reintentar la conexión desde cero.
- **Fallo crítico** (LoRa no inicializa, corrupción de memoria, watchdog timeout): ejecuta `ESP.restart()` para reiniciar el microcontrolador completo, volviendo a S0.

---

## Resumen de estados

| ID | Nombre | Color | Función principal |
|---|---|---|---|
| S0 | Boot | Gris | Inicializar hardware |
| S1 | WiFi Connect | Azul | Conectar a la red local |
| — | Sin WiFi | Ámbar | Operación degradada sin red |
| S2 | APRS-IS Connect | Azul | Autenticarse en la red APRS |
| S3 | Idle / Escucha | Teal | Recibir paquetes LoRa |
| S4 | Decodificar APRS | Púrpura | Parsear y validar el frame |
| S5 | Forward APRS-IS | Púrpura | Subir el paquete a internet |
| S6 | Beacon TX | Coral | Transmitir posición GPS |
| S7 | Error / Watchdog | Rojo | Manejar fallos y reiniciar |

