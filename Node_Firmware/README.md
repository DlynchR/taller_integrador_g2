# Agro-LoRa Node — Diseño Completo del Sistema

> **Nodo de Monitoreo Agrícola de Bajo Consumo (Agro-LoRa)**
>
> Documento de diseño estructurado en 5 niveles siguiendo la guía del proyecto.
>
> Autores: Denzel y Álvaro
>
> Fecha: 2026
>
> Link a presentación: https://canva.link/yzzocvnaaeviv1d

---

## LEVEL 1 — Sistema a Alto Nivel

### Descripción del Sistema

El **Agro-LoRa Node** es un nodo remoto autónomo que mide condiciones ambientales y de suelo,
y transmite los datos a un gateway ubicado a ~2 km de distancia mediante LoRa.

El sistema opera desde una batería LiPo y permanece en **Deep Sleep ≥ 95% del tiempo**
para maximizar la autonomía.

### Inputs Físicos

| Input                  | Tipo   | Descripción                                    |
|------------------------|--------|------------------------------------------------|
| Humedad de suelo         | Analógico | Sensor capacitivo de humedad de suelo          |
| Temperatura / Humedad | I2C    | Sensor ambiental (SHT31)                       |
| Voltage de Batería        | ADC    | Divisor resistivo interno del Feather ESP32    |


### Output RF

Transmisión LoRa a 915 MHz hacia un gateway con ID fijo.

### Diagrama ASCII

```
                  +---------------------+
                  |    [ Ambiente ]      |
                  |  Suelo, Aire, Sol    |
                  +--------+------------+
                           |
              Humedad de suelo (Analog)
              Temp/Humedad (I2C)
              Nivel de Batería (ADC)
                           |
                           v
              +------------------------+
              |                        |
              |   Agro-LoRa Node       |
              |   (Feather ESP32 +     |
              |    RFM95W)             |
              |                        |
              +----------+-------------+
                         |
                    LoRa RF @ 915 MHz
                    SF10 / BW 500 kHz
                         |
                         v
              +------------------------+
              |        Gateway         |
              |   2 km de distancia    |
              +------------------------+
```

---

## LEVEL 2 — Bloques Funcionales

### Subsistemas Principales

#### 1. Manejo de potencia
- Batería LiPo 3.7V (1000–2000 mAh)
- Regulador integrado del Feather ESP32 (3.3V)
- Control de energía de sensores vía GPIO + MOSFET/transistor
- Deep Sleep del ESP32 (~10 µA)

#### 2. Adquisición de datos
- **Humedad de Suelo**: Sensor capacitivo → ADC del ESP32
- **Temp/Humidity**: SHT31 → I2C bus
- **Battery**: Divisor resistivo interno → ADC (VOLTAGE_MONITOR)

#### 3. Procesamiento (MCU)
- Adafruit Feather ESP32 V2
- Máquina de estados finita (FSM) controla el ciclo de operación
- CircuitPython como entorno de ejecución

#### 4. Comunicación RF 
- RFM95W (LoRa Feather)
- SPI bus hacia el MCU
- Protocolo con ACK y reintentos

### Diagrama ASCII de Bloques

```
+-----------------------------------------------------------------------+
|                         Agro-LoRa Node                                |
|                                                                       |
|  +-----------------+        +-------------------+                     |
|  | Power Mgmt      |        | Data Acquisition  |                     |
|  |                 |        |                   |                     |
|  | LiPo Battery    |  3.3V  | Humedad de Suelo ----+---> ADC             |
|  | 3.7V ---------> |------->| SHT31 -----------+---> I2C              |
|  | Reg. (onboard)  |        | Battery Monitor --+---> ADC             |
|  |                 |  GPIO  |                   |                    |
|  | Sensor Power <---+-------| Power Control     |                     |
|  +-----------------+        +--------+----------+                     |
|                                      |                                |
|                                      | Datos (variables internas)     |
|                                      v                                |
|                             +--------+----------+                     |
|                             | Processing (MCU)  |                     |
|                             |                   |                     |
|                             | Feather ESP32 V2  |                     |
|                             | FSM + Deep Sleep  |                     |
|                             +--------+----------+                     |
|                                      |                                |
|                                      | SPI                            |
|                                      v                                |
|                             +--------+----------+                     |
|                             | RF Communication  |                     |
|                             |                   |                     |
|                             | RFM95W LoRa       |                     |
|                             | 915 MHz           |                     |
|                             +--------+----------+                     |
|                                      |                                |
+--------------------------------------+--------------------------------+
                                       |
                                   RF Output
                                       |
                                       v
                               [ Gateway @ 2 km ]
```

---

## LEVEL 3 — Detalles de Interconexión

### Buses y Señales

#### SPI Bus (MCU ↔ RFM95W)

| Señal   | Dirección        | Pin MCU     | Pin RFM95W | Descripción                 |
|----------|------------------|-------------|------------|-----------------------------|
| SCK      | MCU → RFM95W     | board.SCK   | SCK        | Serial Clock                |
| MOSI     | MCU → RFM95W     | board.MOSI  | MOSI       | Master Out, Slave In        |
| MISO     | RFM95W → MCU     | board.MISO  | MISO       | Master In, Slave Out        |
| CS       | MCU → RFM95W     | board.D10   | NSS        | Chip Select (active low)    |
| RESET    | MCU → RFM95W     | board.D11   | RESET      | Hardware Reset              |
| DIO0/IRQ | RFM95W → MCU     | board.D6    | DIO0       | Interrupt (TX/RX complete)  |

#### I2C Bus (MCU ↔ SHT31)

| Señal | Dirección     | Pin MCU     | Pin SHT31 | Descripción         |
|--------|---------------|-------------|-----------|---------------------|
| SDA    | Bidireccional | board.SDA   | SDA       | Serial Data         |
| SCL    | MCU → SHT31   | board.SCL   | SCL       | Serial Clock        |

#### Analog Inputs

| Señal         | Dirección     | Pin MCU  | Descripción                          |
|----------------|---------------|----------|--------------------------------------|
| Humedad de suelo  | Sensor → MCU  | board.A1 | ADC lectura 0–65535                  |
| Nivel de batería  | Interno → MCU | board.VOLTAGE_MONITOR | Divisor resistivo interno |

#### GPIO Control

| Señal         | Dirección  | Pin MCU   | Descripción                              |
|-----------------|-----------|-----------|------------------------------------------|
| Sensor Power    | MCU → Out | board.D12 | Control MOSFET para energía de sensores  |
| LED Status      | MCU → Out | board.D13 | LED indicador de actividad               |

### Diagrama ASCII de Interconexión

```
                         Feather ESP32 V2
                    +------------------------+
                    |                        |
 Humedad de Suelo +--->| A1 (ADC)               |
 Sensor        |    |                        |        RFM95W FeatherWing
               |    |                        |       +-------------------+
               |    | SCK  (SPI) ----------->|------>| SCK               |
               |    | MOSI (SPI) ----------->|------>| MOSI              |
               |    | MISO (SPI) <-----------|<------| MISO              |
               |    | D10  (CS)  ----------->|------>| NSS               |
               |    | D11  (RST) ----------->|------>| RESET             |
               |    | D6   (IRQ) <-----------|<------| DIO0              |
               |    |                        |       |              [ Antenna ]
               |    |                        |       +-------------------+
               |    |                        |
               |    | SDA (I2C) <----------->|<----->| SDA  |  SHT31
               |    | SCL (I2C) ------------>|------>| SCL  |  Temp/Hum
               |    |                        |       +------+
               |    |                        |
               |    | D12 (GPIO) ----------->| ---> [MOSFET Gate]
               |    |                        |           |
               |    | D13 (GPIO) ----------->| ---> [LED]|
               |    |                        |           v
               |    | VOLTAGE_MONITOR (ADC)  |     [Sensor VCC]
               |    |   (internal)           |
               |    |                        |
               |    | BAT <--- LiPo 3.7V     |
               |    +------------------------+
```

**Nota sobre el MOSFET**: Se utiliza un N-channel MOSFET (ej. 2N7000 o IRLML6344)
en el lado LOW de los sensores. Cuando `D12 = HIGH`, el MOSFET conduce y los sensores
reciben energía. En Deep Sleep, `D12` vuelve a LOW y los sensores se apagan completamente.

El SHT31 también se alimenta a través del mismo MOSFET para garantizar consumo cero durante sleep.

---

## LEVEL 4 — Lógica de Firmware

### PART A: Flowchart (Energy-focused)

El ciclo principal está diseñado para minimizar el tiempo activo.
El nodo despierta cada `SLEEP_INTERVAL` segundos (por defecto 300s = 5 min).

```
                            ( START )
                                |
                                v
                    +------------------------+
                    | Inicializar hardware   |
                    | SPI, I2C, GPIO, LoRa   |
                    +------------------------+
                                |
                                v
                    +------------------------+
                    | Power ON sensores      |
                    | D12 = HIGH             |
                    +------------------------+
                                |
                                v
                    +------------------------+
                    | Esperar estabilización |
                    | time.sleep(0.2s)       |
                    +------------------------+
                                |
                                v
                    +------------------------+
                    | Leer Humedad de Suelo  |
                    | Leer SHT31 (T, H)      | 
                    | Leer Battery Level     |
                    +------------------------+
                                |
                                v
                    +------------------------+
                    | Power OFF sensores     |
                    | D12 = LOW              |
                    +------------------------+
                                |
                                v
                    +------------------------+
                    | Formatear paquete      |
                    | "SM/TEMP/HUM/BAT"      |
                    +------------------------+
                                |
                                v
                    +------------------------+
                    | Transmitir via LoRa    |
                    | send_with_ack()        |
                    +----------+-------------+
                               |
                               v
                        < ACK recibido? >
                       /                \
                     YES                 NO
                      |                   |
                      v                   v
              +-------------+    +------------------+
              | Blink LED   |    | retry_count += 1 |
              +------+------+    +--------+---------+
                     |                    |
                     |                    v
                     |           < retry < MAX? >
                     |           /              \
                     |         YES               NO
                     |          |                 |
                     |          v                 |
                     |   [Retransmitir]           |
                     |       (loop)               |
                     |                            |
                     +----------------------------+
                               |
                               v
                    +------------------------+
                    | Entrar en Deep Sleep   |
                    | alarm = TimeAlarm      |
                    | (SLEEP_INTERVAL)       |
                    +------------------------+
                               |
                               v
                           ( SLEEP )
                               |
                          (Timer fires)
                               |
                               v
                           ( START )
```

### PART B: FSM

#### Estados

| Estado        | Descripción                                          | Timeout    |
|---------------|------------------------------------------------------|------------|
| SLEEP         | Deep Sleep, consumo mínimo (~10 µA)                  | 300 s      |
| SENSING       | Sensores encendidos, lectura de datos                | 5 s        |
| TX_DATA       | Transmisión del paquete LoRa                         | 2 s        |
| WAIT_ACK      | Espera de confirmación del gateway                   | 5 s        |
| ERROR_RETRY   | Manejo de fallo, reintento o abandono                | 1 s        |

#### Transiciones

| Estado Actual  | Condición                              | Acción                      | Estado Siguiente |
|----------------|----------------------------------------|-----------------------------|------------------|
| SLEEP          | SLEEP_INTERVAL expirado (timer alarm)  | init_hardware               | SENSING          |
| SENSING        | Todos los sensores leídos OK           | power_off_sensors           | TX_DATA          |
| SENSING        | Timeout 5s sin lectura completa        | power_off_sensors, log_error| ERROR_RETRY      |
| TX_DATA        | Paquete enviado (send_with_ack)        | start_ack_timer             | WAIT_ACK         |
| TX_DATA        | Fallo SPI / error de transmisión       | log_error                   | ERROR_RETRY      |
| WAIT_ACK       | ACK recibido correctamente             | blink_led                   | SLEEP            |
| WAIT_ACK       | Timeout 5s sin ACK                     | increment_retry             | ERROR_RETRY      |
| ERROR_RETRY    | retry_count < MAX_RETRIES (3)          | re-send                     | TX_DATA          |
| ERROR_RETRY    | retry_count >= MAX_RETRIES             | log_failure                 | SLEEP            |

#### Diagrama de Estados ASCII

```
                    +-------------------+
                    |                   |
           +------->     SLEEP         |
           |        |  (Deep Sleep)    |
           |        +--------+---------+
           |                 |
           |          timer_alarm (300s)
           |                 |
           |                 v
           |        +--------+---------+
           |        |                  |
           |        |    SENSING       |<----------+
           |        |  (Read sensors)  |           |
           |        +--------+---------+           |
           |                 |                     |
           |          data_ready / timeout(5s)     |
           |            |              |           |
           |            v              |           |
           |   +--------+--------+    |           |
           |   |                 |    |           |
           |   |    TX_DATA      |    |           |
           |   | (LoRa transmit) |<---+-----+     |
           |   +--------+--------+    |     |     |
           |            |             |     |     |
           |         packet_sent      |     |     |
           |            |             |     |     |
           |            v             |     |     |
           |   +--------+--------+   |     |     |
           |   |                 |   |     |     |
           |   |   WAIT_ACK      |   |     |     |
           |   | (Espera ACK)    |   |     |     |
           |   +--------+--------+   |     |     |
           |       |          |      |     |     |
           |   ack_ok    timeout(5s) |     |     |
           |       |          |      |     |     |
           |       |          v      v     |     |
           |       |   +------+------+-+   |     |
           |       |   |               |   |     |
           |       |   | ERROR_RETRY   |---+     |
           |       |   | (retry/fail)  | retry<MAX
           |       |   +-------+-------+         |
           |       |           |                  |
           +-------+           |                  |
           ack_ok        retries_exhausted        |
           or retries                             |
           exhausted                              |
```

**Garantía de seguridad**: TODOS los estados tienen condición de salida.
No hay forma de que el sistema quede atrapado en un estado de alto consumo:
- `SENSING` tiene timeout de 5s → `ERROR_RETRY`
- `TX_DATA` tiene timeout de 2s → `ERROR_RETRY`
- `WAIT_ACK` tiene timeout de 5s → `ERROR_RETRY`
- `ERROR_RETRY` siempre termina en `TX_DATA` o `SLEEP`





## LEVEL 5 — Diseño de Hardware

### 1. Selección de componentes

| Componente      | Modelo                     | Justificación                                                    |
|-----------------|----------------------------|------------------------------------------------------------------|
| MCU             | Adafruit Feather ESP32 V2  | Deep Sleep ~10 µA, WiFi/BLE (futuras mejoras), CircuitPython, Feather form factor |
| LoRa Module     | RFM95W (FeatherWing)       | Compatible Feather, SX1276, 915 MHz.   |
| Humedad de Suelo   | Sensor capacitivo genérico | Salida analógica, sin electrólisis, bajo consumo (~5 mA)        |
| Temp/Humidity   | SHT31 (breakout I2C)       | Precisión ±0.3°C / ±2% RH, I2C, bajo consumo (~0.8 mA active)  |
| Reg. Voltage     | Integrado en Feather ESP32 | AP2112K-3.3, 600mA max, dropout 250mV                           |
| MOSFET          | IRLML6344 (N-ch)           | Vgs_th ~1V, Rds_on ~29mΩ, SOT-23, compatible 3.3V logic        |
| Batería         | LiPo 3.7V, 2000 mAh       | Conector JST-PH , compatible Feather                            |
| Antena         | Cable monopolo λ/4 (8.2 cm)| Económica, suficiente para 2 km con SF10                        |

### 2. Asignación de pines

| Function         | Pin Board     | GPIO ESP32  | Notas                                |
|------------------|---------------|-------------|--------------------------------------|
| SPI_SCK          | board.SCK     | GPIO5       | Clock hacia RFM95W                   |
| SPI_MOSI         | board.MOSI    | GPIO19      | Data out hacia RFM95W                |
| SPI_MISO         | board.MISO    | GPIO21      | Data in desde RFM95W                 |
| LORA_CS          | board.D10     | GPIO15      | Chip Select RFM95W (active low)      |
| LORA_RST         | board.D11     | GPIO27      | Reset RFM95W                         |
| LORA_IRQ         | board.D6      | GPIO14      | DIO0 interrupt (TX/RX done)          |
| I2C_SDA          | board.SDA     | GPIO22      | Bus datos SHT31                      |
| I2C_SCL          | board.SCL     | GPIO20      | Bus clock SHT31                      |
| SOIL_ADC         | board.A1      | GPIO25      | Lectura analógica Humedad de Suelo      |
| BAT_ADC          | board.VOLTAGE_MONITOR | — | Divisor interno batería              |
| SENSOR_PWR       | board.D12     | GPIO12      | Gate del MOSFET para sensores        |
| LED_STATUS       | board.D13     | GPIO13      | LED indicador en PCB                 |

### 3. Estrategia de manejo de potencia

#### Sensor Power ON/OFF

Los sensores (Humedad de Suelo + SHT31) se alimentan a través de un **N-channel MOSFET (IRLML6344)**
conectado en el lado LOW (entre GND del sensor y GND del sistema).

```
3.3V Rail ----+----> Sensor VCC
              |
          [Sensor]
              |
          Sensor GND
              |
         +----+----+
         |  Drain   |
         | IRLML6344|
         |  Source   |
         +----+----+
              |
             GND

Gate <--- D12 (GPIO12) con pull-down 10kΩ a GND
```

- **D12 = HIGH** → MOSFET conduce → sensores reciben energía
- **D12 = LOW** (o en Deep Sleep) → MOSFET no conduce → consumo ~0
- Pull-down resistor en el gate asegura que los sensores se apaguen durante deep sleep
  (GPIOs flotan en deep sleep en ESP32)

#### Estrategia de Deep Sleep 

1. El ESP32 entra en Deep Sleep usando `alarm.time.TimeAlarm`
2. Al despertar, el ESP32 ejecuta un **hard reset** (re-ejecuta `code.py` desde el inicio)
3. No se preserva estado en RAM (no es necesario para este caso de uso)
4. El RFM95W entra automáticamente en sleep cuando el ESP32 duerme

#### Duty Cycle Estimado

| Phase             | Duración | Porcentaje |
|-------------------|----------|------------|
| Active (sense+TX) | ~3 s     | 1.0%       |
| Deep Sleep        | 297 s    | 99.0%      |
| **Total Cycle**   | **300 s**|  ≥ 95%     |

### 4. Estimación de consumo de potencia

| Estado            | Componente    | Corriente    | Fuente / Justificación                    |
|-------------------|---------------|--------------|--------------------------------------------|
| **Deep Sleep**    | ESP32         | ~10 µA       | Datasheet ESP32-PICO-V3-02                 |
|                   | RFM95W (idle) | ~0.2 µA      | SX1276 datasheet (sleep mode)              |
|                   | Sensores      | ~0 µA        | MOSFET apagado                             |
|                   | **Total Sleep** | **~10 µA** |                                            |
| **Active Sensing**| ESP32 (CPU)   | ~40 mA       | Single core, 80 MHz                        |
|                   | SHT31         | ~0.8 mA      | Measurement mode                           |
|                   | Soil sensor   | ~5 mA        | Típico sensor capacitivo                   |
|                   | **Total Sense** | **~46 mA** |                                            |
| **TX LoRa**       | ESP32 (CPU)   | ~40 mA       | Procesando SPI                             |
|                   | RFM95W TX     | ~28 mA       | 13 dBm, SX1276 datasheet                  |
|                   | **Total TX**  | **~68 mA**   |                                            |

#### Estimación de Vida de Batería

```
Corriente promedio = (3s × 57mA + 297s × 0.01mA) / 300s
                   = (171 + 2.97) / 300
                   ≈ 0.58 mA promedio

Batería 2000 mAh:
    Vida ≈ 2000 / 0.58 ≈ 3448 horas ≈ 144 días
```

> **Nota**: Esta es una estimación optimista. En la práctica, el factor de auto-descarga
> de la batería y la eficiencia del regulador reducen este valor un ~20–30%.
> Estimación realista: **~100–110 días**.

### 5. Consideraciones de Hardware

#### Conversión de Niveles Lógicos
- **No se requiere**. El Feather ESP32 opera a 3.3V, el RFM95W a 3.3V, y el SHT31 a 3.3V.
  Todos los componentes son compatibles en nivel lógico.

#### Aislamiento de Sensores (MOSFET)
- Se usa **IRLML6344** (N-channel, logic-level), Vgs_th = 1.0V típico.
- A 3.3V en gate, el MOSFET conduce completamente (Rds_on ~29 mΩ).
- Pull-down de 10 kΩ en gate para asegurar apagado durante deep sleep.
- Un solo MOSFET controla todos los sensores (corriente total < 10 mA, bien dentro de los 5A del MOSFET).

#### Antena
- **Antena de cable monopolo λ/4**: largo = 300/(915×4) ≈ **8.2 cm**
- Soldada directamente al pad de antena del RFM95W FeatherWing
- Justificación: económica, fácil de implementar, suficiente ganancia para 2 km con SF10
- Alternativa futura: antena SMA para mayor flexibilidad

#### Protección
- La batería LiPo tiene protección integrada en el Feather ESP32 (MCP73831 charger IC)
- No se requiere protección adicional de sobre-voltaje en las entradas analógicas
  (el sensor capacitivo opera a 3.3V, dentro del rango del ADC)

---

## Requerimientos de diseño de LORA

### Parámetros Configurados

| Parámetro          | Valor        | Justificación                                           |
|--------------------|--------------|---------------------------------------------------------|
| **Frequency**      | 915.0 MHz    | ISM banda para las Américas (FCC Part 15.247)           |
| **Spreading Factor** | SF10       | Balance entre alcance y tiempo en aire. SF10 da ~15 dB de link budget adicional vs SF7, suficiente para 2 km con obstáculos agrícolas |
| **Bandwidth**      | 500 kHz      | Reduce time-on-air  |
| **TX Power**       | 13 dBm       | Mínimo necesario para 2 km. Reduce consumo vs 20+ dBm  |
| **TX Interval**    | 300 s (5 min)| Suficiente para monitoreo agrícola.    |

### Justificación Detallada

**Distancia (2 km)**:
Con SF10 y BW 500 kHz, la sensibilidad del RFM95W es ~-134 dBm.
Link budget: 13 dBm TX + 0 dBi (monopolo) - path_loss(2km, 915MHz) ≈ 13 - 92 = -79 dBm.
Margen: -79 - (-134) = **55 dB de margen**, más que suficiente incluso con obstáculos.

**Power Consumption**:
El time-on-air para un paquete de ~30 bytes con SF10/BW500kHz es ~10 ms.
Con intervalo de 300 s, el duty cycle RF es 10ms/300s = 0.003%, muy eficiente.

**Duty Cycle**:
La regulación FCC para 915 MHz ISM permite hasta 1 segundo de transmisión
por cada 20 segundos (frequency hopping) o operación de baja potencia limitada.
Nuestro diseño transmite ~10 ms cada 300 s, cumpliendo ampliamente.



