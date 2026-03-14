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
