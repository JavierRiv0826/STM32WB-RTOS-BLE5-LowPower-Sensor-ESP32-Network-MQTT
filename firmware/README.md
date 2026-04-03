# 🌍 Distributed BLE Low-Power Sensor Network with Wi-Fi MQTT Gateway

A complete low-power IoT sensing system built around:

- 🔋 STM32WB55 BLE Sensor Node (battery-powered, ultra-low-power)
- 🌐 ESP32 BLE → WiFi → MQTT Gateway
- ☁ Cloud connectivity via MQTT broker

This project demonstrates modern embedded system design across heterogeneous MCUs, combining:

- Ultra-low-power firmware architecture (Stop2 mode)
- BLE 5.0 custom GATT implementation
- Dual-core STM32WB architecture usage
- ESP-IDF modular component architecture
- BLE Central ↔ Peripheral communication
- Event-driven RTOS systems
- Protocol bridging (BLE → MQTT)
- Real-time system monitoring (OLED diagnostics)

---

# 🧠 System Overview

The system consists of two primary embedded devices:

1. **Sensor Node (STM32WB55)**  
   Battery-powered BLE 5.0 environmental & motion sensor.

2. **Gateway Node (ESP32)**  
   BLE Central device that forwards sensor data to the cloud via MQTT.

<p align="center">
  <img src="/docs/images/ble_mqtt_sensor_network.png" width="1000"/>
  <br>
  <em>CAN System with RTOS Gateway & BLE</em>
</p>

---

# 🏗 Global Architecture

```bash
STM32WB (BLE Peripheral)
│
│  GATT Notifications / Indications
▼
ESP32 (BLE Central)
│
│  Event-driven forwarding
▼
MQTT Broker (Cloud)
│
▼
Dashboard / Android / Backend
```

The architecture separates:

- Edge sensing (ultra-low-power domain)
- Network connectivity (Wi-Fi domain)
- Cloud integration (MQTT domain)

This layered separation improves scalability, reliability, and energy efficiency.

---

# 🔋 Sensor Node — STM32WB55 BLE Low-Power Device

## 📌 Hardware

- MCU: STM32WB55CGU6 (Dual-core M4 + M0+ BLE)
- Power: CR2450 Coin Cell
- PIR Sensor: AM312 (Interrupt-driven)
- Environmental Sensors:
    - BMP280 (Pressure + Temperature)
    - AHT20 (Humidity + Temperature)
- Clocking:
    - LSE 32.768 kHz (RTC + BLE timing)
    - SMPS Enabled for efficiency

---

## ⚙ Firmware Architecture

FreeRTOS-based layered design:

- Sensor Task
- BLE Task
- Power Manager Task
- OTA Placeholder Task

Inter-task communication via:
- Event flags
- Timer Server callbacks
- BLE stack event handlers

---

## 🔋 Power Strategy

The node operates primarily in STOP2 mode.

```bash
STOP2 (Idle)
↓  (RTC / PIR interrupt)
RUN (Measurement + BLE TX)
↓
STOP2
```

### Wake-up Sources

- PIR interrupt (motion detection)
- RTC periodic wakeup
- BLE stack events

### Power Characteristics

- Sleep current ≈ 20 µA
- Active burst ≈ 10–15 mA (short duration)
- Average current ≈ 23 µA

Estimated CR2450 lifetime:

~1.5 to 2.5 years (real-world conditions)

---

## 📡 BLE GATT Profile

Custom 16-bit Vendor Service (UUID: 0xA000)

### Characteristics

| Characteristic | Property | Purpose                        |
|----------------|----------|--------------------------------|
| Motion Count   | Indicate | Reliable motion event delivery |
| Environmental  | Notify   | Temp, Humidity, Pressure       |
| Battery Level  | Notify   | 0–100% percentage              |

### Environmental Payload (8 bytes)

```bash
int16  temperature (°C ×100)
uint16 humidity (% ×100)
uint32 pressure (Pa)
```

Design goal: minimize RF airtime and power consumption.

---

## 🧠 Embedded Engineering Concepts Demonstrated

- Dual-core STM32WB architecture
- BLE Indication acknowledgment handling
- Stop2 low-power integration with RTOS
- Interrupt-driven sensing
- Packed binary data transport
- Battery-aware firmware design

---

# 🌐 Gateway Node — ESP32 BLE → Wi-Fi → MQTT Bridge

## 📌 Hardware

- MCU: ESP32-WROOM-HW-394
- BLE + WiFi coexistence
- SH1106 OLED display (I2C)
- Power: 5V USB

---

## 🧠 Firmware Architecture (ESP-IDF)

Modular component-based design:

- wifi_manager
- mqtt_manager
- ble_central
- oled_display
- system_monitor
- system_events
- i2c_bus
- common

FreeRTOS Event Groups synchronize system states:

- WIFI_CONNECTED_BIT
- MQTT_CONNECTED_BIT
- BLE_CONNECTED_BIT

---

# 🔁 Gateway Runtime State Machine

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

Reconnection Logic:

- Wi-Fi drop → MQTT stopped
- MQTT drop → retry
- BLE disconnect → restart scan

---

# 📡 BLE Central Operation

### Scanning

- Active scan
- Filters by device name: "STM32WB"
- Auto-connect to known nodes

### GATT Discovery

- Primary service discovery
- Characteristic discovery
- CCCD enabling sequence
- Subscription to:
    - Environmental notifications
    - Motion indications
    - Battery notifications

---

# ☁ MQTT Publishing Model

Each measurement is published independently:

```bash
gateway/sensor/temperature
gateway/sensor/humidity
gateway/sensor/pressure
gateway/sensor/motion
gateway/sensor/battery

QoS: 1  
Retain: false  
Payload: scalar string value
```

This avoids JSON overhead and reduces processing latency.

---

# 🖥 System Observability

OLED displays:

- Wi-Fi state
- MQTT state
- BLE connection state
- Heap diagnostics

Serial monitor logs detailed connection flow.

---

# 🔄 End-to-End Data Flow

```bash
PIR interrupt →
BLE Indication →
ESP32 parses →
MQTT publish →
Broker →
Cloud subscriber

RTC wake →
Environmental/Battery sampling →
BLE Notification →
ESP32 →
MQTT publish
```

The system is fully event-driven with no polling loops.

---

# 🧪 How to Test the Full System

1. Flash STM32WB BLE stack (CPU2)
2. Flash STM32WB application (CPU1)
3. Flash ESP32 gateway (ESP-IDF)
4. Power sensor node (CR2450)
5. Power ESP32 via USB
6. Observe:
    - BLE connection established
    - Wi-Fi connected
    - MQTT connected
7. Trigger PIR motion
8. Observe MQTT topic updates
9. Disconnect Wi-Fi to test reconnection logic

---

# 🧩 Hardware Summary

| Device      | MCU         | Role                      |
|-------------|-------------|---------------------------|
| Sensor Node | STM32WB55   | BLE Peripheral            |
| Gateway     | ESP32-WROOM | BLE Central + WiFi + MQTT |

---

# 🎯 Engineering Highlights

- Ultra-low-power BLE sensing node
- Dual-core STM32WB usage
- Deterministic RTOS design
- BLE Indications for reliability
- Protocol bridge architecture
- Modular ESP-IDF firmware design
- Event-driven synchronization
- BLE/WiFi coexistence stability tuning
- Long-term battery optimization
- Clean layered system architecture

---

# 📚 What This Project Demonstrates

This repository showcases real-world embedded systems engineering:

- Low-power wireless design
- BLE stack integration
- RTOS task architecture
- Multi-protocol IoT bridging
- Cloud integration via MQTT
- Cross-platform MCU interoperability
- Production-style firmware modularization

---

# 👤 Author

Javier Rivera  
GitHub: JavierRiv0826