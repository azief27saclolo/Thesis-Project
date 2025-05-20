#ifndef SOFTWARE_POWER_MONITOR_H
#define SOFTWARE_POWER_MONITOR_H

#include <Arduino.h>

class SoftwarePowerMonitor {
  private:
    // Timing measurements
    unsigned long lastInferenceTime = 0;
    unsigned long inferenceTimeTotal = 0;
    int inferenceCount = 0;
    unsigned long lastCheckpoint = 0;
    
    // Power estimation constants based on ESP32-CAM benchmarks
    // These are rough approximations in milliamps
    const float IDLE_CURRENT_MA = 70.0;      // ESP32-CAM idle current
    const float CAMERA_CURRENT_MA = 120.0;   // Additional current when camera active
    const float WIFI_TX_CURRENT_MA = 250.0;  // Additional current during WiFi transmission
    const float ML_BASE_CURRENT_MA = 180.0;  // Base ML inference current
    const float ML_PER_MS_CURRENT_MA = 0.5;  // Additional current per ms of inference
    
    // Power tracking
    float totalPowerUsage_mAh = 0.0;
    unsigned long lastPowerUpdate = 0;
    float cameraActiveTime_ms = 0;
    float wifiActiveTime_ms = 0;
    float inferenceActiveTime_ms = 0;
    
  public:
    void begin() {
      lastCheckpoint = millis();
      lastPowerUpdate = millis();
    }
    
    // Call when starting camera capture
    void startCamera() {
      updatePowerUsage();
      lastCheckpoint = millis();
    }
    
    // Call when camera capture ends
    void endCamera() {
      unsigned long duration = millis() - lastCheckpoint;
      cameraActiveTime_ms += duration;
      updatePowerUsage();
      lastCheckpoint = millis();
    }
    
    // Call when starting WiFi transmission
    void startWifiTransmission() {
      updatePowerUsage();
      lastCheckpoint = millis();
    }
    
    // Call when WiFi transmission ends
    void endWifiTransmission() {
      unsigned long duration = millis() - lastCheckpoint;
      wifiActiveTime_ms += duration;
      updatePowerUsage();
      lastCheckpoint = millis();
    }
    
    void startInference() {
      updatePowerUsage();
      lastInferenceTime = millis();
    }
    
    void endInference() {
      unsigned long duration = millis() - lastInferenceTime;
      inferenceTimeTotal += duration;
      inferenceActiveTime_ms += duration;
      inferenceCount++;
      
      // Calculate power for this inference
      float inferencePower_mAh = calculateInferencePower(duration);
      
      Serial.printf("Inference took %lu ms (est. %0.2f mAh)\n", duration, inferencePower_mAh);
      updatePowerUsage();
      lastCheckpoint = millis();
    }
    
    float calculateInferencePower(unsigned long duration_ms) {
      // Convert ms to hours
      float duration_h = duration_ms / 3600000.0;
      
      // Calculate power (ML_BASE_CURRENT + additional current based on time)
      float power_mAh = (ML_BASE_CURRENT_MA + (ML_PER_MS_CURRENT_MA * duration_ms)) * duration_h;
      return power_mAh;
    }
    
    void updatePowerUsage() {
      unsigned long now = millis();
      unsigned long idleTime = now - lastPowerUpdate;
      float idleTime_h = idleTime / 3600000.0; // Convert ms to hours
      
      // Update idle power usage
      totalPowerUsage_mAh += IDLE_CURRENT_MA * idleTime_h;
      
      lastPowerUpdate = now;
    }
    
    // Get estimated total power usage in mAh
    float getTotalPowerUsage() {
      updatePowerUsage(); // Update with latest
      return totalPowerUsage_mAh;
    }
    
    // Get estimated battery runtime in hours based on current usage pattern
    float getEstimatedRuntime(float batteryCapacity_mAh = 2000.0) {
      if (millis() == 0) return 0; // Avoid division by zero
      
      // Calculate average power consumption per hour
      float elapsedTime_h = millis() / 3600000.0;
      float avgPowerPerHour = totalPowerUsage_mAh / elapsedTime_h;
      
      // Estimate remaining runtime
      return batteryCapacity_mAh / avgPowerPerHour;
    }
    
    float getAverageInferenceTime() {
      return inferenceCount > 0 ? (float)inferenceTimeTotal / inferenceCount : 0;
    }
    
    float getAverageInferencePower() {
      if (inferenceCount == 0) return 0;
      float avgTime = getAverageInferenceTime();
      return calculateInferencePower(avgTime);
    }
    
    // Get ML model memory usage (static approximation based on model size)
    int getModelMemoryUsage() {
      // This is a rough approximation - return kB
      return 350; // For MobileNetV2 with alpha=0.25
    }
    
    // Print power stats to Serial
    void printPowerStats() {
      Serial.println("\n--- Power Usage Statistics ---");
      Serial.printf("Total estimated power usage: %.2f mAh\n", getTotalPowerUsage());
      Serial.printf("ML inference count: %d\n", inferenceCount);
      Serial.printf("Avg inference time: %.1f ms\n", getAverageInferenceTime());
      Serial.printf("Avg inference power: %.3f mAh\n", getAverageInferencePower());
      Serial.printf("ML model memory: %d kB\n", getModelMemoryUsage());
      
      // Activity breakdown
      Serial.printf("Camera active time: %.1f sec\n", cameraActiveTime_ms/1000);
      Serial.printf("WiFi TX active time: %.1f sec\n", wifiActiveTime_ms/1000);
      Serial.printf("ML inference time: %.1f sec\n", inferenceActiveTime_ms/1000);
      Serial.println("-----------------------------\n");
    }
};

#endif
