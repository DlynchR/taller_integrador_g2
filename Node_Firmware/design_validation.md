## ✔️ Design Validation

### Regla 1: El sistema NO puede quedarse en estado de alto consumo

✅ **Verificado**: Todos los estados activos tienen timeouts:
- `SENSING`: timeout 5s → `ERROR_RETRY`
- `TX_DATA`: timeout 2s → `ERROR_RETRY`
- `WAIT_ACK`: timeout 5s → `ERROR_RETRY`
- `ERROR_RETRY`: máximo 3 reintentos → `SLEEP`

El peor caso activo es: 5s + 3×(2s + 5s + 1s) = 5 + 24 = **29 segundos**.
Incluso en el peor caso, el sistema vuelve a Deep Sleep.

### Regla 2: Todos los estados tienen condición de salida

✅ **Verificado**: Ver tabla de transiciones de la FSM.
Cada estado tiene al menos una transición de salida (por éxito o por timeout).

### Regla 3: Los sensores NUNCA están energizados durante sleep

✅ **Verificado**:
- `power_off_sensors()` se llama ANTES de cualquier transición fuera de `SENSING`
- El pull-down de 10 kΩ en el gate del MOSFET asegura que los sensores se apaguen
  incluso si el GPIO flota durante Deep Sleep
- El MOSFET está en el lado LOW: sin señal de gate = no conduce = sensores sin energía

### Regla 4: La falla de comunicación está manejada

✅ **Verificado**:
- `send_with_ack()` retorna `True/False`
- Si falla → `ERROR_RETRY` con hasta `MAX_RETRIES = 3` intentos
- Si todos los reintentos fallan → el nodo entra a Deep Sleep y reintenta en el próximo ciclo
- No hay pérdida catastrófica: los datos del próximo ciclo se transmitirán

### Regla 5: No hay forma de consumo parásito

✅ **Verificado**:
- SPI CS queda HIGH (deseleccionado) en Deep Sleep
- I2C bus queda inactivo (SHT31 consume < 1 µA en idle, y está sin poder vía MOSFET)
- LED queda apagado (D13 = LOW por defecto en Deep Sleep)