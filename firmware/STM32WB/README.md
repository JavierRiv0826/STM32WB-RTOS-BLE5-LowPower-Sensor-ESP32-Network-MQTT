# 🔋 STM32WB BLE Low-Power Motion Sensor Node 

<p align="center">
  <em>STM32WB55 BLE Sensor Node (CR2450 Powered)</em>
</p>

---

## 📌 Description

A low-power wireless sensor node built on the **STM32WB55CGU6** (WeAct Studio board), featuring **BLE 5.0 communication**, **Stop2 ultra-low-power mode**, and **interrupt-driven motion detection**.

This node is designed for battery-powered distributed sensing applications and operates on a **CR2450 coin cell**. It advertises sensor data via a custom BLE GATT service and wakes only on motion or scheduled RTC events.

The firmware follows a modular RTOS-based layered architecture:

- FreeRTOS task separation
- Custom BLE GATT service abstraction
- Stop2 power management
- PIR interrupt-driven motion detection
- Periodic environmental monitoring
- Periodic battery voltage monitoring

This project demonstrates ultra-low-power embedded design, BLE stack integration on STM32WB, and event-driven IoT node architecture.

<p align="center">
  <img src="../docs/images/stm32wb/sensor_node.png" width="500"/>
  <br>
  <em>Low-Power BLE Sensor Node  </em>
</p>

---

# 🚀 Features

## 🕵️ Motion Detection
- ✔ PIR sensor interrupt (EXTI)
- ✔ Interrupt-driven wake from Stop2
- ✔ Motion event counter
- ✔ BLE indication on motion

## 🌡 Environmental Monitoring
- ✔ BMP280 (Barometric pressure + temperature)
- ✔ AHT20 (Humidity + temperature)
- ✔ Packed environmental BLE characteristic (8-byte payload)
- ✔ Periodic RTC-based sampling
- ✔ BLE notification for environmental data

## 📡 BLE 5.0 Communication
- ✔ Custom 16-bit vendor-specific GATT service (UUID: 0xA000)
- ✔ Motion count characteristic (Indicate – reliable delivery)
- ✔ Environmental characteristic (Notify – low overhead)
- ✔ Battery voltage characteristic (Notify)
- ✔ 10-second advertising window
- ✔ Auto-stop advertising for power saving
- ✔ Indication acknowledgment handling

## 🔋 Ultra-Low Power Design
- ✔ Stop2 mode between events
- ✔ LSE 32.768 kHz for RTC timing
- ✔ Timer Server wake-up
- ✔ BLE stack low-power integration
- ✔ CR2450 coin cell optimized operation

## 🧠 FreeRTOS Architecture
- ✔ Sensor task
- ✔ BLE task
- ✔ Power manager task
- ✔ OTA placeholder task
- ✔ Event flag synchronization

---

# ⚙️ STM32CubeMX Configuration

## 🧠 MCU
- STM32WB55CGU6
- Dual-core (M4 + M0+ for BLE stack)
- BLE 5.0 capable

## 🔄 Clock Configuration
- LSE = 32.768 kHz (RTC + BLE timing)
- HSE = 32 MHz (board default)
- SMPS = Enabled (recommended for efficiency)
- MSI used for low-power transitions

Optimized for low-power wireless operation.

## 🔁 I2C1 Configuration

I2C1 is used to communicate with the environmental sensors (BMP280, AHT20).

* Instance: I2C1
* Timing register: `0x00B07CB4` (CubeMX-generated, ~100 kHz Standard Mode)
* Addressing Mode: 7-bit
* Dual Address Mode: Disabled
* General Call: Disabled
* No Stretch Mode: Disabled
* Analog Filter: Enabled
* Digital Filter: Disabled (coefficient = 0)
* Own Address1: 0x00 (master mode)

The peripheral is initialized via:

```c
HAL_I2C_Init(&hi2c1);
```

Analog and digital filters are configured using:

```c
HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE);
HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0);
```

---

## ↔️ GPIO Configuration

GPIO configuration is defined in `MX_GPIO_Init()`.

### 🟢 Output Pins (Status LEDs, only for debugging)

Configured as:
* Mode: Push-Pull Output (`GPIO_MODE_OUTPUT_PP`)
* Pull: No Pull
* Speed: Low Frequency

| Pin          | Function                  |
|--------------|---------------------------|
| LED_INT_Pin  | Motion activity indicator |
| LED_RTC_Pin  | RTC wake-up indicator     |
| LED_Stop_Pin | STOP mode indicator       |
| LED_BLUE_Pin | Advertising indicator     |

All LEDs are initialized to **RESET (LOW)** state at startup.

⚠️ **Important:**  
These LEDs are used **for debugging purposes only**.

They significantly increase current consumption and are **not intended for normal low-power operation**.  
For accurate power measurements and production deployment, LED usage should be disabled in firmware.

---

### 🟣 PIR Motion Interrupt

| Pin         | Configuration                     |
|-------------|-----------------------------------|
| PIR_INT_Pin | External interrupt on Rising Edge |

Configured as:

* Mode: `GPIO_MODE_IT_RISING`
* Pull: No pull
* NVIC Priority: 5
* IRQ Enabled: `EXTI0_IRQn`

Interrupt is enabled via:

```c
HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
HAL_NVIC_EnableIRQ(EXTI0_IRQn);
```

This interrupt triggers a BLE indication event.

---

# 🔋 Power Strategy
The node operates in the following state machine:
```bash
+---------+
| STOP2   |
| (Idle)  |
+---------+
↓ 
↓ (RTC / PIR wakeup)
+---------+
| Measure |
+---------+
↓ 
↓ 
+---------+
| STOP2   |
+---------+
```

## Wake-up Sources
- PIR interrupt (motion)
- RTC periodic wakeup (environmental/battery sampling)
- BLE stack events

## Power Modes

| Mode              | Description                        |
|-------------------|------------------------------------|
| RUN               | Active measurement + BLE           |
| BLE Advertising   | RF active                          |
| STOP2             | Ultra-low-power sleep              |

---

# 🔋 Estimated Battery Life

The node is optimized for long-term CR2450 operation.

Typical assumptions:

- Sleep current: ~20 µA
- Periodic measurement every 5 minutes
- Active time: ~100 ms @ 10 mA
- BLE burst peaks: 12–15 mA (short duration)

Average current ≈ 23 µA

Estimated theoretical lifetime:

620 mAh / 0.023 mA ≈ 3.1 years

Considering temperature effects, aging, and coin cell pulse limitations:

**Expected real-world lifetime: 1.5 – 2.5 years**

See full electrical and current analysis in:

👉 `/docs/Sensor_Node_Technical_Documentation.md`

---

# 📡 BLE GATT Profile

## Custom Service

The node exposes a custom 16-bit vendor-specific GATT service (UUID: 0xA000) containing the following characteristics:

| Characteristic     | Properties | Description                              |
|--------------------|------------|------------------------------------------|
| Motion Count       | Indicate   | Incremented on PIR event                 |
| Environmental Data | Notify     | Temperature, humidity, pressure (packed) |
| Battery (VBAT)     | Notify     | Measured via internal ADC                |

---

## 🌡 Environmental Data Characteristic

Environmental data is aggregated from:

- **BMP280** → Pressure + secondary temperature
- **AHT20** → Humidity + primary temperature

To reduce BLE overhead, values are transmitted as a packed 8-byte payload:

| Field       | Type   | Scaling   |
|-------------|--------|-----------|
| Temperature | int16  | °C × 100  |
| Humidity    | uint16 | %RH × 100 |
| Pressure    | uint32 | Pa        |

**Total payload size:** 8 bytes

Environmental data is sent using **BLE Notifications** to minimize RF airtime and power consumption.

---

## 🕵️ Motion Characteristic

Motion events use **BLE Indications** to ensure reliable delivery.

Indications are acknowledged by the client, making them suitable for discrete motion events where data loss is unacceptable.

The firmware confirms each indication via the  
`ACI_GATT_INDICATION_VSEVT_CODE` event from the STM32WB BLE stack.

---

## 🔋 Battery Characteristic

The battery voltage is measured using the STM32WB internal VBAT channel and exposed via:

- UUID: `0x2A19` (Standard BLE Battery Level)
- Format: `uint8`
- Unit: Percentage (0–100%)

This allows remote monitoring of CR2450 battery health.

---

# 🔌 Hardware Connections

## 🕵️ PIR Sensor (AM312 → STM32WB)

| PIR Pin | STM32WB                 |
|---------|-------------------------|
| VCC     | 3.3V*                   |
| GND     | GND                     |
| OUT     | GPIO (EXTI Rising Edge) |

Configured as interrupt source to wake from Stop2.

## 🔌 Wiring (AHT20 → STM32WB)

| AHT20 | STM32F103 |
|-------|-----------|
| VCC   | 3.3V*     |
| GND   | GND       |
| SDA   | PB9       |
| SCL   | PB8       |

I2C Address: 0x38

## 🔌 Wiring (BMP280 → STM32WB)

| BMP280 | STM32F103 |
|--------|-----------|
| VCC    | 3.3V*     |
| GND    | GND       |
| SDA    | PB9       |
| SCL    | PB8       |

I2C Address: 0x77

Both sensors share the same I2C bus.

#### * Powered directly from CR2450 battery. Nominal voltage: 3.0V (2.0–3.2V operating range).
---

# 🧩 Firmware Architecture
<p align="center">
  <img src="../docs/images/stm32wb/sensor_node_architecture.png" width="200"/>
  <br>
  <em>Firmware Architecture</em>
</p>

---

# 🗂 Project Structure
```bash
project/
├── Core/
│   ├── Inc/
│   │   ├── adc.h
│   │   ├── app_config.h
│   │   ├── FreeRTOSConfig.h
│   │   ├── gpio.h
│   │   └── i2c.h
│   └── Src/
│       ├── adc.c
│       ├── app_freertos.c
│       ├── gpio.c
│       ├── i2c.c
│       └── stm32wbxx_it.c
│
├── STM32_WPAN/
│   └── App/
│       ├── app_ble.c
│       └── app_ble.h
│
├── Sensors/
│   ├── aht20/
│   │   ├── aht20.c
│   │   └── aht20.h
│   └── bmp280/
│       ├── bmp280.c
│       └── bmp280.h
│
├── ble/
│   ├── ble_service.c
│   └── ble_service.h
│
└── rtos/
    ├── aSensorTask/
    │   ├── sensor_task.c
    │   └── sensor_task.h
    │
    ├── bBLETask/
    │   ├── ble_task.c
    │   └── ble_task.h
    │
    ├── cPowerTask/
    │   ├── power_manager.c
    │   └── power_manager.h
    │
    └── dOTATask/
        ├── ota_task.c
        └── ota_task.h
```

## 📁 Directory Overview

### 🔹 Core
HAL drivers, MCU configuration, interrupt handlers, and FreeRTOS base configuration.

### 🔹 STM32_WPAN
ST BLE stack integration layer and application entry points.

### 🔹 Sensors
Low-level drivers for environmental sensors:
- AHT20 (temperature & humidity)
- BMP280 (pressure)

### 🔹 ble
Custom BLE service implementation (GATT profile and characteristic handling).

### 🔹 rtos
Application-level FreeRTOS tasks:
- `aSensorTask` → Environmental acquisition
- `bBLETask` → BLE event processing & notifications
- `cPowerTask` → STOP2 management & wake logic
- `dOTATask` → Firmware update handling

---

# 💡 How to Run 🧪

## 1️⃣ Clone Repository
```bash
git clone https://github.com/JavierRiv0826/STM32WB-RTOS-BLE5-LowPower-Sensor-ESP32-Network-MQTT.git
```

## 2️⃣ Flash Wireless Stack (CPU2 - M0+)

Before running the project, ensure the BLE Full Stack firmware is installed on CPU2.

Steps:

1. Open **STM32CubeProgrammer**
2. Connect via **ST-Link**
3. Go to:
   ```
   Firmware Upgrade Services
   ```
4. Install the appropriate **BLE Full Stack** binary  
   (e.g., `stm32wb5x_BLE_Stack_full_fw.bin`)
5. Verify successful installation

This step is required only once on a blank device.

---

## 3️⃣ Open in STM32CubeIDE

File → Open Projects from File System

---

## 4️⃣ Build Project

`Ctrl + B`

---

## 5️⃣ Flash MCU (CPU1 - M4)

Connect **ST-Link** → Click **Run** or **Debug**

This flashes the application firmware to the Cortex-M4 core.

---

## 6️⃣ Scan via BLE App

Use:

- nRF Connect
- ble_monitor.py
- STM32WB custom Android app

Connect to the device and discover the custom GATT service.

---

## 7️⃣ Trigger Motion (PIR)

Move in front of the PIR sensor.

Observe:

- Motion Count characteristic sends an **Indication**
- Client acknowledges the indication
- Counter increments

---

## 8️⃣ Environmental & Battery Updates

After connection at periodic interval:

Observe:

- Environmental characteristic sends **Notification**
    - Temperature
    - Humidity
    - Pressure
- Battery characteristic sends **Notification**
    - Reported as percentage (0–100%)

---

# 🧠 Engineering Highlights

- Dual-core STM32WB architecture usage  
- BLE GATT custom implementation  
- Stop2 ultra-low-power integration  
- RTOS-based task separation  
- Indication acknowledgment handling  
- Battery-aware wireless node design  

---

# 🛠 Development Tools

- STM32CubeIDE  
- STM32CubeMX  
- nRF Connect (BLE debugging)  

---

# 👤 Author

**Javier Rivera**  
GitHub: JavierRiv0826
