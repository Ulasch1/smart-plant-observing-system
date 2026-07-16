/*
 * Project: Smart Plant Observing System
 * Features: 
 * - Data Optimization (Reduces bandwidth usage)
 * - Fault Detection (Detects sensor disconnection)
 * - State-Based Notification System (Prevents spam)
 * - Wi-Fi Telemetry (RSSI Monitoring)
 */

#include "secrets.h" // Şifrelerin ve token'ların olduğu dosyayı dahil ediyoruz

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// --- HARDWARE PIN DEFINITIONS ---
const int sensorPowerPin = 5;     // GPIO 5 for Sensor VCC (Corrosion protection)
const int sensorSignalPin = 34;   // GPIO 34 for Analog Input

// --- CALIBRATION DATA (Potting Soil / Peat Moss) ---
const int DRY_VALUE = 4095;       // Air value (0% Moisture)
const int WET_VALUE = 1000;       // Water saturated value (100% Moisture)

// --- THRESHOLDS & FLAGS ---
const int CRITICAL_MOISTURE_LEVEL = 30; // Trigger warning below 30%
const int HYSTERESIS_RESET_VAL = 10;    // Reset alarm when moisture increases by +10% (at 40%)
bool isWaterWarningActive = false;      // Flag to prevent notification spam

const int FAULT_THRESHOLD = 4090;       // Values above this indicate disconnection
bool isFaultMode = false;               // Flag to track sensor health

// --- Wi-Fi & Blynk Credentials ---
BlynkTimer timer;

// Variables to store last sent data for optimization
int lastMoisture = -1;
int lastSignal = -100;

// --- 1. SENSOR READ FUNCTION (With Smoothing) ---
int readSensor() {
  digitalWrite(sensorPowerPin, HIGH); // Power on the sensor
  delay(50); // Wait for voltage to stabilize

  long total = 0;
  // Take 10 readings and average them to remove noise
  for (int i = 0; i < 10; i++) {
    total += analogRead(sensorSignalPin);
    delay(10);
  }
  digitalWrite(sensorPowerPin, LOW); // Power off to prevent corrosion

  return total / 10; // Return raw average
}

// --- 2. SYSTEM ANALYSIS & LOGIC CORE ---
void runSystemLogic() {
  int rawValue = readSensor();
  int wifiSignal = WiFi.RSSI(); // Get Wi-Fi signal strength (dBm)

  // --- A) FAULT DETECTION LOGIC ---
  if (rawValue > FAULT_THRESHOLD) {
    if (!isFaultMode) {
      Serial.println("ERROR: Sensor Disconnected! (Check Cable connection)");
      Blynk.logEvent("sensor_fault", "Sensor data lost! Check hardware connection.");
      isFaultMode = true; // Enter fault mode
    }
    Blynk.virtualWrite(V1, 0); 
    return; // Stop execution here
  } else {
    isFaultMode = false; // System is healthy
  }

  // --- B) DATA PROCESSING ---
  int moisturePercent = map(rawValue, DRY_VALUE, WET_VALUE, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100); // Keep within bounds

  // --- C) DATA OPTIMIZATION (Send only on change) ---
  if (abs(moisturePercent - lastMoisture) >= 1 || abs(wifiSignal - lastSignal) >= 5) {
    Blynk.virtualWrite(V1, moisturePercent); // Update Moisture Graph
    Blynk.virtualWrite(V2, wifiSignal);      // Update Signal Strength
    
    Serial.print("Moisture: %"); Serial.print(moisturePercent);
    Serial.print(" | Signal: "); Serial.print(wifiSignal); Serial.println("dBm");
    
    lastMoisture = moisturePercent;
    lastSignal = wifiSignal;
  }

  // --- D) SMART NOTIFICATION SYSTEM (State Machine) ---
  if (moisturePercent < CRITICAL_MOISTURE_LEVEL && !isWaterWarningActive) {
    Blynk.logEvent("water_warning", String("Plant needs water! Level: %") + moisturePercent);
    Serial.println("WARNING: Low moisture notification sent.");
    isWaterWarningActive = true; // Lock to prevent repeated alerts
  }
  
  if (moisturePercent > (CRITICAL_MOISTURE_LEVEL + HYSTERESIS_RESET_VAL) && isWaterWarningActive) {
    isWaterWarningActive = false;
    Serial.println("INFO: Watering detected. Alarm system reset.");
  }
}

void setup() {
  Serial.begin(115200);
  
  // Pin Configuration
  pinMode(sensorPowerPin, OUTPUT);
  digitalWrite(sensorPowerPin, LOW);

  // Connect to Blynk (secrets.h içinden değişkenleri çeker)
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Run system logic every 2 seconds
  timer.setInterval(2000L, runSystemLogic);
}

void loop() {
  Blynk.run();
  timer.run();
}