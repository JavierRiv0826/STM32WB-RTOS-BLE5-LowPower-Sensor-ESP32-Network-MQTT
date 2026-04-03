# 🌐 ESP32 BLE → WiFi MQTT Gateway

<p align="center">
  <em>ESP32-WROOM-HW-394 BLE Central Gateway with WiFi + MQTT + OLED Monitoring</em>
</p>

---

## 📌 Description

A modular IoT gateway built on the **ESP32-WROOM-HW-394**, acting as a **BLE Central device** that connects to a BLE environmental sensor node (STM32WB) and bridges its data to the cloud via **Wi-Fi + MQTT**.

The firmware is developed using **ESP-IDF**, following a clean component-based architecture with:

- BLE Central scanner & client
- Wi-Fi connection manager
- MQTT client abstraction
- SH1106 OLED system monitor
- Event-driven internal architecture (FreeRTOS Event Groups)
- Modular component separation

It demonstrates BLE–Wi-Fi protocol bridging, embedded modular design in ESP-IDF, and real-time system observability via OLED display.

<p align="center">
  <img src="/docs/images/esp32/ble_gateway.png" width="500"/>
  <br>
  <em>ESP32 BLE–MQTT Gateway</em>
</p>

---

# 🚀 Features

## 📡 BLE Central Mode
- ✔ Active BLE scanning
- ✔ Automatic connection to known sensor node ("STM32WB")
- ✔ Full GATT service & characteristic discovery
- ✔ Dynamic CCCD subscription handling
- ✔ Notification parsing (temperature, humidity, pressure, motion, battery)
- ✔ Automatic BLE reconnection logic

## 🌐 WiFi Connectivity
- ✔ Station mode
- ✔ Auto-reconnect on disconnect
- ✔ Event-based connection handling
- ✔ Wi-Fi power-save disabled for BLE coexistence stability

## ☁ MQTT Cloud Bridge
- ✔ MQTT client abstraction layer
- ✔ Auto-reconnect tied to Wi-Fi state
- ✔ QoS 1 publishing
- ✔ Topic-based routing
- ✔ Broker-agnostic implementation

## 🖥 OLED System Display (SH1106)
- ✔ Real-time WiFi status
- ✔ MQTT connection status
- ✔ BLE connection status

## 🧠 System Monitoring
- ✔ FreeRTOS task watchdog integration
- ✔ Connection state monitoring
- ✔ Event-driven synchronization
- ✔ Heap diagnostics display

## 🧩 Modular ESP-IDF Component Architecture
- Clean separation of concerns
- Reusable components
- Independent build units
- Scalable design for additional nodes

---

# 🧠 System Architecture

                ┌────────────────────┐
                │   STM32WB Node     │
                │  BLE Peripheral    │
                └─────────┬──────────┘
                          │ Notifications
                          ▼
                ┌────────────────────┐
                │   BLE Central      │
                │   (ESP32)          │
                └─────────┬──────────┘
                          │ Parsed Data
                          ▼
                ┌────────────────────┐
                │   Event Layer      │
                │ (Event Groups)     │
                └─────────┬──────────┘
                          │
                          ▼
                ┌────────────────────┐
                │   MQTT Manager     │
                └─────────┬──────────┘
                          │
                          ▼
                ┌────────────────────┐
                │   MQTT Broker      │
                └────────────────────┘

The ESP32 operates as a protocol bridge:

BLE GATT → Internal Event Bus → MQTT Publish

---

# 🧩 Firmware Architecture

The project follows a component-based ESP-IDF structure.

Each major subsystem is implemented as an independent component:

- `wifi_manager` → WiFi state machine
- `mqtt_manager` → MQTT lifecycle & publish abstraction
- `ble_central` → BLE scanner + GATT client
- `oled_display` → High-level UI rendering
- `sh1106` → Low-level OLED driver
- `i2c_bus` → I2C abstraction
- `system_monitor` → Watchdog & runtime stats
- `system_events` → FreeRTOS Event Group synchronization
- `common` → Shared data types

Communication between modules is event-driven using centralized system event bits:

- WIFI_CONNECTED_BIT
- MQTT_CONNECTED_BIT
- BLE_CONNECTED_BIT

---

# ⚙️ ESP-IDF Configuration

## 🧠 MCU
- ESP32-WROOM-HW-394
- Dual-core Xtensa LX6
- Integrated WiFi + BLE
- FreeRTOS SMP

## 🔄 Bluetooth Configuration
- NimBLE stack
- Central mode
- GATT client enabled
- Active scanning
- Notifications + Indications supported

## 🌐 WiFi Configuration
- Station Mode
- Event loop integrated
- Auto reconnect enabled
- Power Save Disabled (WIFI_PS_NONE)

## ☁ MQTT Configuration
- TCP transport
- Public broker (HiveMQ)
- QoS 1 publishing
- Keep-alive enabled
- Clean session configurable

---

# 📡 BLE Operation

## Scanning Phase
- Active scan enabled
- Scan interval: 0x0010
- Scan window: 0x0010
- Filters by device name: "STM32WB"
- Connects automatically to known nodes

## GATT Discovery
- Primary service discovery
- Characteristic discovery
- Dynamic CCCD enabling sequence
- Subscription to:
    - Temperature notification
    - Humidity notification
    - Pressure notification
    - Motion indication
    - Battery notification

## Data Handling

Incoming BLE packets are:

1. Parsed
2. Published to MQTT topic
3. Optionally displayed on OLED

Example parsing:
- int16 temperature → float (/100)
- uint16 humidity → float (/100)
- uint32 pressure
- uint8 motion
- uint8 battery %

---

# ☁ MQTT Data Publishing

Sensor data is published per measurement using a topic-based structure:
```bash
gateway/sensor/temperature
gateway/sensor/humidity
gateway/sensor/pressure
gateway/sensor/motion
gateway/sensor/battery
```

Each topic publishes a scalar string value:

```bash
Topic:   gateway/sensor/temperature
Payload: "23.45"
QoS:     1
Retain:  false
```
This design provides:

Fine-grained subscription control<br>
Reduced payload size<br>
No JSON parsing overhead<br>
Event-driven publishing directly from BLE notifications<br>

The MQTT manager abstracts all broker communication from the BLE layer, ensuring modular separation between transport logic and sensor processing.
Publishing is triggered directly from BLE notification callbacks, resulting in low-latency, event-driven data forwarding without polling.

---

# 🖥 OLED Display (SH1106)

The OLED provides live diagnostics:

| Screen Section | Information              |
|----------------|--------------------------|
| WiFi           | Connected / Disconnected |
| MQTT           | Broker status            |
| BLE            | Connected node           |

I2C communication handled via dedicated `i2c_bus` component.

## 🔌 Hardware Connections - Wiring (SH1106 → ESP32)

| SH1106 | ESP32 |
|--------|-------|
| VCC    | 3.3V  |
| GND    | GND   |
| SDA    | D21   |
| SCL    | D22   |

I2C Address: 0x3C

<p align="center">
  <img src="/docs/images/esp32/oled_display.jpeg" width="200"/>
  <br>
  <em>OLED Display (SH1106)</em>
</p>

---

# 🔁 Runtime State Machine

```bash
BOOT
↓
WiFi Connecting
↓
WiFi Connected
↓
MQTT Connecting
↓
MQTT Connected
↓
BLE Scanning
↓
BLE Connected
↓
Data Streaming
```

Reconnection logic implemented for:
- Wi-Fi drops → MQTT stopped
- MQTT disconnect → retry
- BLE link loss → restart scan

---

# 📁 Project Structure

```bash
ble_gateway/
│
├── CMakeLists.txt
├── sdkconfig
│
├── main/
│   ├── CMakeLists.txt
│   └── main.c
│
└── components/
    ├── wifi_manager/
    │   ├── CMakeLists.txt
    │   ├── wifi_manager.c
    │   └── include/
    │       └── wifi_manager.h
    │
    ├── mqtt_manager/
    │   ├── CMakeLists.txt
    │   ├── mqtt_manager.c
    │   └── include/
    │       └── mqtt_manager.h
    │
    ├── ble_central/
    │   ├── CMakeLists.txt
    │   ├── ble_central.c
    │   └── include/
    │       └── ble_central.h
    │
    ├── oled_display/
    │   ├── CMakeLists.txt
    │   ├── oled_display.c
    │   └── include/
    │       └── oled_display.h
    │
    ├── sh1106/
    │   ├── CMakeLists.txt
    │   ├── sh1106.c
    │   └── include/
    │       └── sh1106.h
    │
    ├── i2c_bus/
    │   ├── CMakeLists.txt
    │   ├── i2c_bus.c
    │   └── include/
    │       └── i2c_bus.h
    │
    ├── system_monitor/
    │   ├── CMakeLists.txt
    │   ├── system_monitor.c
    │   └── include/
    │       └── system_monitor.h
    │
    ├── system_events/
    │   ├── CMakeLists.txt
    │   ├── system_events.c
    │   └── include/
    │       └── system_events.h
    │
    └── common/
        ├── CMakeLists.txt
        └── include/
            └── data_types.h
```

---

# 📁 Directory Overview

### 🔹 main
Application entry point.  
Initializes subsystems and orchestrates startup sequence.

### 🔹 wifi_manager
Handles Wi-Fi initialization, connection, and event handling.

### 🔹 mqtt_manager
Encapsulates ESP-IDF MQTT client and publish/subscribe logic.

### 🔹 ble_central
Implements BLE scanning, connection, GATT client, and notification handling.

### 🔹 oled_display
High-level UI rendering logic.

### 🔹 sh1106
Low-level OLED driver implementation.

### 🔹 i2c_bus
I2C abstraction for shared peripherals.

### 🔹 system_monitor
Watchdog feeding and runtime diagnostics.

### 🔹 system_events
FreeRTOS Event Group synchronization layer.

### 🔹 common
Shared data types and project-wide structures.

---

# 💡 How to Run 🧪

## 1️⃣ Install ESP-IDF

Follow official setup guide from Espressif.

## Clone Repository
```bash
git clone https://github.com/JavierRiv0826/STM32WB-RTOS-BLE5-LowPower-Sensor-ESP32-Network-MQTT.git
```

---

## 2️⃣ Configure Project
```bash
idf.py menuconfig
```
* Configure:
```bash
Component config →
    Bluetooth →
        Host → NimBLE - Enable
Component config →
    ESP-MQTT Configuration →
        Enable MQTT protocol
Component config →
    Wi-Fi Provisioning Manager →
        Enable WiFi provisioning
```

---

## 3️⃣ Build
```bash
idf.py build
```

---

## 4️⃣ Flash

```bash
idf.py -p COM9 flash monitor
```

---

## 5️⃣ Observe System

- OLED shows connection state
- Serial monitor prints debug logs
- MQTT broker receives sensor data
  
Use:
- ble_monitor.py
- STM32WB custom Android  (MQTT mode)

---

# 🧠 Engineering Highlights

- Production-style BLE Central implementation
- BLE → WiFi → MQTT protocol bridge
- Modular ESP-IDF component architecture
- Event-driven system synchronization
- FreeRTOS watchdog integration
- BLE/WiFi coexistence stability tuning
- Real-time OLED diagnostics
- Sequential CCCD enabling with callback chaining
- Clean separation between transport layer (MQTT) and acquisition layer (BLE)

---

# 🛠 Development Tools

- ESP-IDF
- VSCode + ESP-IDF extension
- HiveMQ broker

---

# 👤 Author

**Javier Rivera**  
GitHub: JavierRiv0826  
