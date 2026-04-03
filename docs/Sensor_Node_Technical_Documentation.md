# Sensor Node Technical Documentation

---

# 1. Architecture

## 1.1 System Overview

The sensor node is a low‑power BLE motion sensor built on STM32WB with FreeRTOS.
The firmware follows a modular and event‑driven architecture.

Main design goals:

* Ultra‑low power operation (Stop2 based)
* Event‑driven BLE communication
* Clean separation between Sensor, BLE, and Power layers
* Scalable task-based architecture

---

## 1.2 High Level Architecture

Application Layers:

* Application Logic
* BLE Service Layer
* Sensor Abstraction Layer
* Power Management Layer
* RTOS Scheduler
* STM32WB HAL + BLE Stack

---

## 1.3 FreeRTOS Tasks

### 1. BLE Task

Handles:

* BLE stack events
* Indication confirmations
* Advertising control
* Connection state tracking

Priority: High
Behavior: Event driven via task notifications

---

### 2. Sensor Task

Handles:

* Periodic battery measurement
* Periodic environmental measurements
* Data update to BLE service

Priority: Medium
Wake source: Timer server (RTC based)

---

### 3. Power Manager Task

Handles:

* Stop2 entry control
* Advertising timeout
* Wake source evaluation
* System state transitions

Priority: Medium

---

### 4. OTA Task (Placeholder)

Reserved for future firmware upgrade support.

---

## 1.4 System State Machine

Primary states:

STOP2<br>
→ Wake (RTC or PIR)<br>
→ MEASURE<br>
→ BLE_NOTIFY<br>
→ (Timeout or Disconnect)<br>
→ STOP2<br>

Wake Sources:

* RTC timer
* PIR interrupt
* BLE event

---

# 2. BLE Profile

## 2.1 Custom Service Overview

A custom BLE GATT service exposes:

1. Motion Counter Characteristic
2. Environmental Characteristic
3. Battery Voltage Characteristic

Service Type: Primary Service<br>
UUID Type: Custom 16-bit vendor-specific GATT service (UUID: 0xA000)

---

## 2.2 Motion Counter Characteristic

Properties:

* Read
* Indicate

Data Format:

* 8-bit unsigned integer

Purpose:
Reports motion events from PIR sensor.

Indications are used instead of notifications to ensure reliable delivery.

---
## 2.3 Environmental Data Characteristic

Properties:

* Read
* Notify

Data Format (packed structure):

* Temperature: 16-bit signed integer (°C × 100)
* Humidity: 16-bit unsigned integer (%RH × 100)
* Pressure: 32-bit unsigned integer (Pa)

Total payload size: 8 bytes

Purpose:
Provides environmental measurements acquired from:

* BMP280 (pressure)
* AHT20 (temperature and humidity)

The packed format minimizes BLE payload size while maintaining adequate precision.


## 2.4 Battery Voltage Characteristic

Properties:

* Read
* Notify

Data Format:

* 8-bit unsigned integer (Unit: Percentage 0–100%)

Purpose:
Reports measured VBAT value.

---

## 2.5 Attribute Count Structure

Service declaration<br>
Characteristic declaration (motion)<br>
Characteristic value (motion)<br>
CCCD (motion)<br>
Characteristic declaration (battery)<br>
Characteristic value (battery)<br>
Characteristic declaration (environmental)<br>
Characteristic value (environmental)<br>
CCCD (environmental, if notify enabled)<br>

Total attributes: 8–9 (depending on notify configuration)

---

## 2.6 Indication Flow

Motion flow:

1. Motion detected
2. Motion counter updated
3. BLE service updated
4. Indication sent if client enabled CCCD
5. Confirmation event handled in BLE task

Environmental flow (if notifications enabled):

1. Periodic RTC wakeup
2. Sensors measured
3. Environmental characteristic updated
4. Notification sent (non-blocking)

Indications are used only for motion events where delivery guarantee is important, while environmental data may use notifications to reduce protocol overhead.

---

# 3. Power Design

## 3.1 Power Strategy Overview

The node is optimized for battery operation using Stop2 low‑power mode.

Design goals:

* Maximize sleep time
* Wake only on relevant events
* Keep RF active only when necessary

---

## 3.2 Low Power Mode Selection

Stop2 was selected because:

* RAM retention is preserved
* RTC remains active
* BLE stack compatible with low power
* Fast wakeup time

Expected current (typical WB55 values):

* Stop2: ~2–5 µA
* BLE advertising: ~5–10 mA peaks
* CPU active: few mA

---

## 3.3 Wake Sources

1. RTC Timer (periodic measurement)
2. PIR GPIO interrupt (motion event)
3. BLE radio events

---

## 3.4 Stop2 Entry Strategy

Stop2 is entered from the FreeRTOS idle hook when:

* No tasks are pending
* BLE is not actively transmitting
* No critical sections active

Procedure:

1. Suspend scheduler tick
2. Enter Stop2
3. Resume tick after wake

---

## 3.5 Advertising Power Optimization

Advertising is:

* Enabled only at start for 10 seconds
* Disabled after timeout
* Not running during Stop2

This minimizes RF energy consumption.

---

## 3.6 Battery Measurement

Battery voltage is measured using:

* Internal VBAT channel
* ADC single conversion
* Converted to percentage (0–100%) before BLE exposure

Measurement is periodic and not continuous to reduce ADC power usage.

---

## 3.7 Future Improvements

* Dynamic advertising interval adjustment
* Motion batching before RF transmission
* Deep battery threshold sleep mode
* Disable advertising after connection

---

## 3.8 Coin Cell Design Analysis (CR2450)

This design targets operation from a CR2450 lithium coin cell.

### Battery Characteristics (CR2450)

Typical specifications:

* Nominal voltage: 3.0 V
* Fresh voltage: ~3.2 V
* End‑of‑life voltage: ~2.0 V
* Capacity: ~600–620 mAh

The STM32WB55 operating range (1.71 V – 3.6 V) fully supports CR2450 operation.

### Peripheral Voltage Compatibility

| Device    | Operating Voltage | Compatibility                 |
|-----------|-------------------|-------------------------------|
| STM32WB55 | 1.71 – 3.6 V      | Fully compatible              |
| BMP280    | 1.71 – 3.6 V      | Fully compatible              |
| AHT20     | 2.0 – 5.5 V       | Compatible (down to ~2.0 V)   |
| AM312 PIR | 2.7 – 12 V        | May stop working below ~2.7 V |

Important consideration:

The AM312 PIR sensor typically requires ≥2.7 V. Near battery end‑of‑life (2.0–2.5 V), the MCU may still operate while the PIR sensor no longer functions reliably.

---

### Sleep Current Estimation

Typical Stop2 system current:

| Device        | Current  |
|---------------|----------|
| STM32WB Stop2 | ~1–2 µA  |
| BMP280 sleep  | ~0.5 µA  |
| AHT20 standby | ~0.25 µA |
| AM312 idle    | ~15 µA   |

Estimated total sleep current:

~17–20 µA

This is acceptable for multi‑year coin cell operation.

---

### Active Current

During measurement phase:

| Device             | Current |
|--------------------|---------|
| STM32WB running    | 5–8 mA  |
| BMP280 measurement | ~0.7 mA |
| AHT20 measurement  | ~0.5 mA |

Peak during active phase: ~7–10 mA

During BLE TX burst:

STM32WB BLE peak ≈ 12–15 mA

---

### Coin Cell Peak Current Limitation

CR2450 typical limits:

* Recommended continuous current: ~2–4 mA
* Short pulses: ~15 mA (very short duration)

BLE transmissions occur in short bursts. A bulk capacitor is required to prevent supply droop.

Recommended decoupling topology:

CR2450
|
100 µF (bulk reservoir)

* 10 µF (local bulk near MCU)
* 100 nF (high‑frequency decoupling)
  |
  STM32WB VDD

Without sufficient bulk capacitance, voltage dips may momentarily drop below 2.0 V and reset the MCU.

---

### Estimated Battery Life

Assumptions:

* Sleep current: 20 µA
* Wake interval: every 5 minutes
* Active time: 100 ms
* Active current: 10 mA

Average current calculation:

Sleep contribution:
20 µA

Active contribution:
10 mA × 0.1 s / 300 s ≈ 3.3 µA

Total average current:
≈ 23 µA

Battery life estimation:

620 mAh / 0.023 mA ≈ 27,000 hours
≈ 3.1 years theoretical

Considering coin cell aging, temperature effects, and pulse inefficiencies:

Expected practical lifetime: 1.5 – 2.5 years

---

### Professional Design Recommendations

1. Place 100 nF capacitor as close as possible to each VDD pin.
2. Add at least 10 µF ceramic close to the MCU supply pins.
3. Use ≥100 µF low‑ESR bulk capacitor near the battery input.
4. Maintain a solid ground plane under the MCU and RF section.
5. Keep RF return path short and symmetric to reduce noise during TX bursts.

This ensures stable BLE operation on a high‑impedance coin cell source.

---

End of Technical Documentation