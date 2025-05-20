#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#include <Arduino.h>

class PowerMonitor {
  private:
    unsigned long lastInferenceTime = 0;
    unsigned long inferenceTimeTotal = 0;
    int inferenceCount = 0;
    unsigned long lastBatteryCheck = 0;
    float initialVoltage = 0.0;
    float currentVoltage = 0.0;
    
    // Battery configuration
    const int BATTERY_PIN = 33;  // GPIO pin connected to battery voltage divider
    const float VOLTAGE_DIVIDER_RATIO = 2.0;  // Example: 100K/100K divider = 2.0
    const float BATTERY_MAX_VOLTAGE = 4.2;    // Full charge voltage
    const float BATTERY_MIN_VOLTAGE = 3.3;    // Cutoff voltage
    
  public:
    void begin() {
      pinMode(BATTERY_PIN, INPUT);
      currentVoltage = getBatteryVoltage();
      initialVoltage = currentVoltage;
      lastBatteryCheck = millis();
    }
    
    float getBatteryVoltage() {
      int rawValue = analogRead(BATTERY_PIN);
      // Convert to voltage (3.3V reference with 12-bit ADC = 4095)
      float voltage = (rawValue / 4095.0) * 3.3 * VOLTAGE_DIVIDER_RATIO;
      return voltage;
    }
    
    int getBatteryPercentage() {
      // Calculate percentage based on voltage
      float percentage = (currentVoltage - BATTERY_MIN_VOLTAGE) / 
                        (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * 100.0;
      percentage = constrain(percentage, 0, 100);
      return (int)percentage;
    }
    
    // Get current voltage for display
    float getCurrentVoltage() {
      return currentVoltage;
    }
    
    void startInference() {
      lastInferenceTime = millis();
    }
    
    void endInference() {
      unsigned long duration = millis() - lastInferenceTime;
      inferenceTimeTotal += duration;
      inferenceCount++;
      
      // Log inference time for power consumption estimation
      Serial.printf("Inference took %lu ms\n", duration);
    }
    
    void checkPowerConsumption() {
      // Check every 30 seconds for display purposes
      if (millis() - lastBatteryCheck > 30000) {
        currentVoltage = getBatteryVoltage();
        float voltageDrop = initialVoltage - currentVoltage;
        float avgInferenceTime = inferenceCount > 0 ? 
                                (float)inferenceTimeTotal / inferenceCount : 0;
        
        Serial.printf("Battery: %.2fV (drop: %.2fV)\n", currentVoltage, voltageDrop);
        Serial.printf("Battery Level: %d%%\n", getBatteryPercentage());
        Serial.printf("Avg inference time: %.1f ms\n", avgInferenceTime);
        Serial.printf("Total inferences: %d\n", inferenceCount);
        
        lastBatteryCheck = millis();
      }
    }
    
    // Note: For battery monitoring without specialized hardware:
    // 1. You MUST connect a voltage divider to GPIO 33:
    //    Battery+ --- [R1 100KΩ] --- GPIO33 --- [R2 100KΩ] --- GND
    // 2. Set VOLTAGE_DIVIDER_RATIO to (R1+R2)/R2 (typically 2.0)
    //    This brings battery voltage (3.7-4.2V) into ESP32's safe range (0-3.3V)
};

#endif
