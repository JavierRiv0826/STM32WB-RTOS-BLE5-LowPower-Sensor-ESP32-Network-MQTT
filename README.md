# 🌍 STM32WB Distributed Low-Power IoT Sensor Network

A full-stack embedded IoT system demonstrating ultra-low-power firmware design, BLE protocol engineering, multi-MCU interoperability, cloud integration, and cross-platform monitoring.

This project integrates:

- 🔋 STM32WB55 BLE Sensor Node (Battery-powered, ultra-low-power, FreeRTOS)
- 🌐 ESP32 Gateway Node (BLE Central → Wi-Fi → MQTT bridge, FreeRTOS)
- 🐍 Python Monitoring Toolkit (BLE + MQTT validation tools)
- 📱 Android App (Jetpack Compose) – Switchable dual-mode BLE/MQTT monitoring

---

# 🧠 System Architecture

<p align="center">
  <img src="/docs/images/sytem_architecture.png" width="1000"/>
  <br>
  <em>Distributed Low-Power IoT Sensor Network</em>
</p>

This layered architecture separates:

- Edge sensing (ultra-low-power domain)
- Network bridging (protocol translation domain)
- Cloud communication (MQTT domain)
- Monitoring & validation tools (application domain)

---

# 🧩 Repository Structure

```bash
STM32WB-RTOS-BLE5-LowPower-Sensor-ESP32-Network-MQTT/
│
├── README.md   ← (You are here)
├── LICENSE
├── .gitignore
│
├── docs/
│   ├── Sensor_Node_Technical_Documentation.md
│   └── images/
│
├── firmware/
│   ├── STM32WB/
│   └── ESP32/
│
├── android/
│
└── python/
```

Each subsystem contains its own detailed technical README.

---

# 🔋 1️⃣ STM32WB Sensor Node

Battery-powered BLE 5.0 environmental + motion sensor.

## Key Characteristics

- MCU: STM32WB55 (Dual-core M4 + M0+ BLE stack)
- FreeRTOS-based architecture
- Stop2 low-power strategy
- Interrupt-driven motion sensing
- Custom 16-bit vendor BLE service
- Binary-packed telemetry payloads
- Average current ≈ 23 µA
- CR2450 battery lifetime ≈ 1.5–2.5 years

## Engineering Focus

- Dual-core BLE architecture
- Low-power state transitions
- Indication acknowledgment handling
- RTOS task synchronization
- Deterministic event-driven firmware

---

# 🌐 2️⃣ ESP32 Gateway Node

Protocol bridge: BLE → WiFi → MQTT

## Features

- ESP-IDF modular component design
- FreeRTOS Event Group synchronization
- BLE Central GATT discovery
- Automatic reconnection strategy
- MQTT QoS 1 publishing
- OLED system diagnostics
- WiFi/BLE coexistence handling

## Runtime State Machine

```bash
BOOT → WiFi → MQTT → BLE Scan → BLE Connected → Streaming
```

## Engineering Focus

- Multi-protocol concurrency
- Event-driven state machine
- Clean modular architecture
- Transport reliability handling
- Real-time system observability

---

# 🐍 3️⃣ Python Monitoring Toolkit

Two independent validation tools:

## 🔵 BLE Monitor

- Direct GATT connection to STM32WB
- Characteristic decoding
- Asyncio-based reconnection logic

## 🟢 MQTT Monitor

- Broker subscription (gateway/sensor/#)
- Topic validation
- End-to-end propagation testing

Purpose:
Layered validation of the entire embedded stack.

---

# 📱 4️⃣ Android Monitoring App

Jetpack Compose application supporting:

- 🔵 Direct BLE mode
- 🌐 Secure MQTT over WSS mode

## Architecture

- MVVM
- Repository abstraction
- Flow-based transport layer
- Hilt dependency injection
- Coroutine-safe connection switching
- Deterministic UI state machine

## Engineering Focus

- Transport abstraction (BLE & MQTT interchangeable)
- Reactive state management
- Descriptor write queue handling
- Clean separation of domain & data layers

---

# 🔄 End-to-End Data Flow

```bash
Motion Interrupt →
STM32WB wakes (Stop2 → Run) →
BLE Indication →
ESP32 parses →
MQTT publish →
Broker →
Android / Python subscriber
```

Fully event-driven.  
No polling loops anywhere in the system.

---

# 🎯 What This Project Demonstrates

- Ultra-low-power wireless design
- BLE GATT protocol-level implementation
- RTOS task architecture
- Multi-MCU interoperability
- Protocol bridging (BLE → MQTT)
- Cloud IoT integration
- Cross-platform client applications
- Layered system validation
- Production-style modular firmware design

---

# 🛠 Technologies Used

## Embedded
- STM32WB55
- ESP32 (ESP-IDF)
- FreeRTOS
- BLE 5.0
- I2C Sensors (BMP280, AHT20)
- PIR motion sensor (AM312)

## Cloud
- MQTT (HiveMQ public broker)
- QoS 1 messaging

## Software
- Python (Bleak, paho-mqtt)
- Android (Kotlin, Jetpack Compose)
- Hilt
- Coroutines / Flow

---

# 📸 Screenshots & Diagrams

System diagrams and screenshots available in:

```
docs/images/
```

---

# 🔐 Security Considerations

Current implementation:

- Public MQTT broker
- No enforced BLE pairing
- No authentication layer

Production improvements would include:

- MQTT over TLS with authentication
- BLE pairing & bonding
- Application-layer validation
- Device whitelisting

---

# 🚀 Why This Project Matters

This repository showcases the ability to design and implement:

- A complete distributed embedded system
- Across heterogeneous MCUs
- With power optimization constraints
- With multi-protocol interoperability
- With production-style architecture
- From firmware to mobile application

---

# 👤 Author

Javier Rivera  
Embedded Systems Engineer  
GitHub: JavierRiv0826
