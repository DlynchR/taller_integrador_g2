
# Módulo: Agro-LoRa Node

# Descripción:
#   Nodo de monitoreo agrícola de bajo consumo para Adafruit Feather ESP32 V2
#   con RFM95W LoRa FeatherWing. Mide humedad de suelo (analógico),
#   temperatura/humedad ambiental (SHT31, I2C) y nivel de batería. Transmite
#   datos proactivamente vía LoRa y entra en Deep Sleep entre ciclos.





# Librerías 

import time
import board
import busio
import digitalio
import alarm
from analogio import AnalogIn
import adafruit_rfm9x
import adafruit_sht31d



# Constantes 


# Intervalo de Sleep (5 minutos)
SLEEP_INTERVAL = 300

# Tiempo de estabilización del sensor después de encenderlo (segundos)
SENSOR_SETTLE_TIME = 0.2

# Máximo número de reintentos para la transmisión LORA
MAX_RETRIES = 3

# Tiempo de espera para la lectura del sensor (segundos)
SENSOR_TIMEOUT = 5

# Retardo entre reintentos (segundos)
RETRY_DELAY = 1



# Configuración de Hardware 


# LED de estado (Pin D13) 
led = digitalio.DigitalInOut(board.D13)
led.direction = digitalio.Direction.OUTPUT
led.value = False

# Control de alimentación de sensores (Pin D12 ->  MOSFET Gate) 
sensor_pwr = digitalio.DigitalInOut(board.D12)
sensor_pwr.direction = digitalio.Direction.OUTPUT
sensor_pwr.value = False  # Sensores apagados al iniciar

# Analog: Humedad del suelo (Pin A1) 
soil_adc = AnalogIn(board.A1)

# Analog: Monitor de batería 
bat_adc = AnalogIn(board.VOLTAGE_MONITOR)

# I2C Bus para SHT31
i2c = busio.I2C(board.SCL, board.SDA)
sht31 = adafruit_sht31d.SHT31D(i2c)



# Feather ESP32 RFM95 Config 


# Frecuencia LORA (915 MHz Banda ISM)
RADIO_FREQ_MHZ = 915.0

# SPI bus
spi = busio.SPI(board.SCK, MOSI=board.MOSI, MISO=board.MISO)

# Pins módulo LoRa (RFM95W FeatherWing)
CS = digitalio.DigitalInOut(board.D10)
RESET = digitalio.DigitalInOut(board.D11)

# Inicializar RFM95W
rfm9x = adafruit_rfm9x.RFM9x(spi, CS, RESET, RADIO_FREQ_MHZ)

# Radio parámetros 
rfm9x.signal_bandwidth = 500000    # 500 kHz
rfm9x.spreading_factor = 10        # SF10
rfm9x.tx_power = 13                # 13 dBm

# Nodes
rfm9x.node = 15           # ID de este nodo
rfm9x.destination = 100   # ID del gateway

# Configuración de confiabilidad
rfm9x.enable_crc = True
rfm9x.ack_retries = 3
rfm9x.ack_delay = 0.2
rfm9x.ack_wait = 2
rfm9x.xmit_timeout = 2



# Funciones 


def blink(delay, times):
    """Flash the status LED."""
    for _ in range(times):
        led.value = True
        time.sleep(delay)
        led.value = False
        time.sleep(delay)


def power_on_sensors():
    """Enable sensor power via MOSFET and wait for stabilization."""
    sensor_pwr.value = True
    time.sleep(SENSOR_SETTLE_TIME)


def power_off_sensors():
    """Cut sensor power via MOSFET."""
    sensor_pwr.value = False


def read_soil():
    """Read raw soil moisture ADC value (0–65535)."""
    return soil_adc.value


def read_environment():
    """Read temperature (°C) and humidity (%RH) from SHT31."""
    temp = sht31.temperature
    hum = sht31.relative_humidity
    return round(temp, 1), round(hum, 1)


def read_battery():
    """Read battery voltage from internal voltage divider."""
    return round((bat_adc.value * 2 * 3.3) / 65536, 2)


def collect_data():
    """
    Power sensors, read all values, power off.
    Returns formatted string: "soil/temp/hum/bat"
    """
    power_on_sensors()

    start = time.monotonic()
    soil = read_soil()
    temp, hum = read_environment()
    bat = read_battery()
    elapsed = time.monotonic() - start

    power_off_sensors()

    if elapsed > SENSOR_TIMEOUT:
        print("WARNING: Sensor read took {:.1f}s".format(elapsed))

    return "{}/{}/{}/{}".format(soil, temp, hum, bat)


def transmit_with_retry(data):
    """
    Transmit data via LoRa with ACK. Retries up to MAX_RETRIES times.
    Returns True if ACK received, False otherwise.
    """
    for attempt in range(1, MAX_RETRIES + 1):
        print("TX attempt {}/{}".format(attempt, MAX_RETRIES))
        ack = rfm9x.send_with_ack(bytes(data, "UTF-8"))
        if ack:
            print("ACK received")
            return True
        print("No ACK, retrying in {}s...".format(RETRY_DELAY))
        time.sleep(RETRY_DELAY)

    print("All retries exhausted")
    return False


def enter_deep_sleep():
    """Configure timer alarm and enter deep sleep."""
    time_alarm = alarm.time.TimeAlarm(monotonic_time=time.monotonic() + SLEEP_INTERVAL)
    alarm.exit_and_deep_sleep_until_alarms(time_alarm)



# Ciclo de operación


# En el primer arranque, se ejecuta un ciclo de sensado-transmisión, luego SLEEP.

print("Agro-LoRa Node awake (node={})".format(rfm9x.node))

# Estado: SENSING
print("Collecting sensor data...")
data = collect_data()
print("Data: {}".format(data))

# Estado: TX_DATA + WAIT_ACK + ERROR_RETRY
success = transmit_with_retry(data)

if success:
    blink(0.1, 2)
else:
    blink(0.05, 5)  # Blink rápido = Indicador de fallo

# Estado: SLEEP
print("Entering deep sleep for {}s...".format(SLEEP_INTERVAL))
enter_deep_sleep()
