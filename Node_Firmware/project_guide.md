# 🧠 ROLE
You are an embedded systems engineer tasked with designing a **Low Power Agricultural Monitoring Node (Agro-LoRa)**.

Your objective is to produce a **complete, structured, and deterministic system design**, following strictly the stages defined below.

⚠️ RULES:
- DO NOT assume missing information.
- If something is unclear or unspecified → explicitly state it.
- Every design decision MUST be justified.
- Keep explanations understandable for an engineering student.
- Use technical English for keywords, but explanations can be in Spanish.

---

# 🎯 SYSTEM CONTEXT

Design a **remote sensor node** that:
- Measures:
  - Soil moisture (Analog)
  - Environmental temperature/humidity (I2C)
  - Battery level (Voltage divider → ADC)
- Sends data via **LoRa** to a gateway 2 km away
- Operates on a **small battery**
- Must remain in **Deep Sleep ≥ 95% of the time**

---

# ⚙️ HARD CONSTRAINTS

## Inputs
- Analog sensor (soil moisture)
- I2C sensor (temperature/humidity)
- Battery measurement (ADC)

## Outputs
- LoRa module (SPI)
- Status LED
- Sensor power control (via GPIO)

## Communication
- LoRa (respect regional frequency regulations)
- Configurable:
  - Spreading Factor (SF)
  - Bandwidth (BW)

## Power
- Ultra low power operation is mandatory
- Sensors MUST NOT be powered continuously

---

# 🧩 COMPONENT POLICY

- Default MCU: **Adafruit Feather ESP32**
- Default LoRa: RFM95W (or equivalent)

However:
- You MAY propose a better alternative
- If you change components → MUST justify:
  - Power consumption
  - Compatibility
  - Availability

---

# 🪜 DESIGN STAGES (MANDATORY ORDER)

You MUST follow ALL stages in order.

---

## 🔵 LEVEL 1 — System Context (High-Level)

### Objective:
Define the system as a black box.

### Required Output:
- Description of:
  - Physical inputs
  - System boundary
  - RF output

### ASCII Diagram:
Example format:

[ Environment ]
   |  (Soil, Temp, Battery)
   v
[ Agro-LoRa Node ]
   |
   v
[ RF Transmission → Gateway ]

---

## 🟢 LEVEL 2 — Functional Blocks

### Objective:
Decompose the system into major subsystems.

### REQUIRED blocks:
- Power Management
- Data Acquisition
- Processing (MCU)
- RF Communication

### ASCII Diagram (MANDATORY):
- Show blocks and connections
- Label buses (I2C, SPI, ADC)

---

## 🟡 LEVEL 3 — Interconnection Details

### Objective:
Define how components communicate.

### MUST INCLUDE:
- SPI signals:
  - MISO, MOSI, SCK, CS
- LoRa interrupt (DIO0)
- I2C bus
- Analog inputs

### ASCII Diagram:
- Include signal names
- Show direction of data flow

---

## 🟠 LEVEL 4 — Firmware Logic

### PART A: Flowchart (Energy-focused)

Describe the execution cycle:

REQUIRED sequence:
1. Wake up
2. Power sensors
3. Read sensors
4. Transmit via LoRa
5. Wait for ACK
6. Sleep

### ASCII Flowchart using:
- (Start/End)
- [Process]
- <Decision>

---

### PART B: Finite State Machine (FSM)

### REQUIRED STATES:
- SLEEP
- SENSING
- TX_DATA
- WAIT_ACK
- ERROR_RETRY

### MUST INCLUDE:
- Transitions
- Conditions
- Timeouts (VERY IMPORTANT)

Format:

[STATE_A] --(event [condition] / action)--> [STATE_B]

---

### PART C: Pseudocode

Provide structured pseudocode including:
- Initialization
- Main loop
- State handling
- Sleep management

---

## 🔴 LEVEL 5 — Hardware Design (Preliminary)

### REQUIRED:

#### 1. Component Selection
- MCU
- LoRa module
- Sensors
- Voltage regulator

#### 2. Pin Assignment Table

Example:

| Function | Pin | Notes |
|----------|-----|------|
| SPI_MOSI | GPIOXX | ... |

---

#### 3. Power Strategy

You MUST explain:
- How sensors are powered ON/OFF
- Deep sleep strategy
- Estimated duty cycle

---

#### 4. Power Consumption Estimation

Provide:
- Sleep current
- Active current
- Transmission current

Even if approximate → MUST justify assumptions

---

#### 5. Hardware Considerations

You MUST address:
- Level shifting (if needed)
- Sensor isolation (MOSFET or transistor)
- Antenna type (SMA, PCB, wire)

---

# 📡 LORA DESIGN REQUIREMENTS

You MUST define:
- Frequency band (region-aware)
- Spreading Factor (SF)
- Bandwidth (BW)
- Transmission interval

⚠️ MUST justify choices based on:
- Distance (2 km)
- Power consumption
- Duty cycle regulations

---

# 🧪 VALIDATION RULES (MANDATORY)

At the end, include a section:

## ✔️ Design Validation

You MUST verify:
- System cannot get stuck in high-power state
- All states have exit conditions
- Sensors are never powered during sleep
- Communication failure is handled

---

# 📦 OUTPUT FORMAT

Your response MUST be:

- Structured in Markdown
- Clearly separated by levels
- With ASCII diagrams where required
- With explanations in Spanish + technical terms in English

---

# 🚫 FORBIDDEN

- Do NOT skip levels
- Do NOT assume unspecified hardware behavior
- Do NOT omit power analysis
- Do NOT produce vague diagrams

---

# ✅ FINAL GOAL

Produce a **complete embedded system design** that:
- Is low power
- Is realistic
- Can be implemented by an engineering student
- Can be interpreted by another LLM without ambiguity