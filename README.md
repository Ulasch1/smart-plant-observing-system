# Smart Plant Observing System (ESP32 IoT)

An energy-efficient, robust, and noise-filtered IoT telemetry system built with an ESP32 microcontroller and Blynk Cloud to monitor soil moisture, track hardware connection health, and optimize network bandwidth.

---

## Engineering Core and Implementations

Resistive soil moisture sensors typically suffer from electrochemical degradation (corrosion) and unstable readings due to electrical noise. This project implements four core software-hardware co-design patterns to solve these problems:

### 1. Pulse-Drive Power Control (Anti-Corrosion)
Standard continuous DC current through soil probes triggers electrolysis, oxidizing metal surfaces and ruining accuracy in weeks.
* **Mechanism:** The sensor's VCC is connected to digital **GPIO 5** instead of a constant 3.3V rail.
* **Algorithm:** The ESP32 pulls GPIO 5 `HIGH` only during a reading cycle, waits exactly `50ms` for voltage stabilization, captures the sample, and immediately pulls the pin `LOW`. This reduces active electrical exposure by **97.5%**, significantly extending probe life.

### 2. Signal Smoothing and Hardware Fault Detection
Raw analog signals from soil probes fluctuate heavily due to EMI (Electromagnetic Interference) and power supply ripples.
* **Smoothing:** The firmware takes 10 sequential readings over a 100ms window and processes their arithmetic average to filter out spike noise.
* **Hardware Fault Detection:** Open-circuit scenarios (disconnected or broken sensor cables) are caught instantly. If the raw ADC reading exceeds the `FAULT_THRESHOLD` (4090), the system triggers a `"sensor_fault"` event on the Blynk console and publishes a telemetry bypass payload.

### 3. Hysteresis-Controlled Notification Guard
Small soil moisture oscillations at boundary levels (e.g., bouncing between 29.9% and 30.1%) cause "notification spam" on users' devices.
* **State Machine:** Low-moisture alerts trigger instantly when soil moisture drops below **30%** (Critical level).
* **Deadband Buffer:** Once triggered, the warning state locks. It is only released (reset) when soil moisture rises past **40%** (Critical Level + 10% Reset Hysteresis), preventing repeated alert spam during boundaries.

### 4. Delta-Based Data Transmission (90% Bandwidth Savings)
Continuous, fixed interval data transmission wastes bandwidth and limits Blynk message quota.
* **Dynamic Transmission:** Telemetry data is pushed to Blynk Cloud ONLY if the absolute delta conditions are met:
  * Moisture change is greater than or equal to 1%
  * Wi-Fi RSSI change is greater than or equal to 5dBm
* This dynamic throttling reduces server requests and overall power usage by up to 90%.

---

## Hardware Setup and Pin Mapping

* **Board:** ESP32-WROOM-32 (DevKitC V4)
* **Sensor:** Resistive Soil Moisture Probe with LM393 Comparator Module
* **IoT Gateway:** Blynk Cloud Core (TCP/IP via Wi-Fi)

| ESP32 GPIO | Connected To | Signal/Power Type | Purpose |
|---|---|---|---|
| **GPIO 34 (ADC1)** | A0 (Analog Out) | Analog Input | Noise-isolated soil telemetry (safe from Wi-Fi ADC2 conflicts) |
| **GPIO 5** | VCC | Digital Output | Pulse-Drive Active Power Management (Active HIGH) |
| **GND** | GND | Ground | Common Ground Reference |

---

## Getting Started and Security Setup

To keep sensitive network credentials out of public source control, this repository utilizes a local header file structure.

### 1. Configure the Secrets File
Create a file named `secrets.h` inside the `/firmware` directory and populate it with your local network configurations:

```cpp
#ifndef SECRETS_H
#define SECRETS_H

#define BLYNK_TEMPLATE_NAME "Smart Plant Observing System"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"
#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"

const char ssid[] = "YOUR_WIFI_SSID";
const char pass[] = "YOUR_WIFI_PASSWORD";

#endif