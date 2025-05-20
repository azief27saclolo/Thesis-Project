// This file must be compiled and uploaded using Arduino IDE
#include "esp_camera.h"
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include "SD_MMC.h"
#include <ArduinoJson.h>

// Camera pins for ESP32-CAM
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// WiFi and Blynk configuration
const char* ssid = "YourWiFiName";
const char* password = "YourWiFiPassword";
char auth[] = "YourBlynkAuthToken";

// Virtual pins for Blynk
#define VPIN_DISEASE     V0
#define VPIN_CONFIDENCE  V1
#define VPIN_TIMESTAMP   V2
#define VPIN_HEALTHY     V3
#define VPIN_EARLY_BLIGHT V4
#define VPIN_LATE_BLIGHT V5
#define VPIN_TYLCV       V6

// TFLite globals
namespace {
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* input = nullptr;
  constexpr int kTensorArenaSize = 150000;
  uint8_t tensor_arena[kTensorArenaSize];
}

#define MAX_OFFLINE_DETECTIONS 100
#define OFFLINE_FILE "/offline_detections.json"

bool sd_available = false;
std::vector<String> offline_detections;

// Disease counters
int healthyCount = 0;
int earlyBlightCount = 0;
int lateBlightCount = 0;
int tylcvCount = 0;

void sendDetectionToBlynk(const char* disease, float confidence) {
    if (Blynk.connected()) {
        Blynk.virtualWrite(VPIN_DISEASE, disease);
        Blynk.virtualWrite(VPIN_CONFIDENCE, confidence);
        Blynk.virtualWrite(VPIN_TIMESTAMP, millis());
        
        // Update counters
        if (strstr(disease, "healthy")) {
            healthyCount++;
            Blynk.virtualWrite(VPIN_HEALTHY, healthyCount);
        }
        else if (strstr(disease, "early_blight") || strstr(disease, "early-blight")) {
            earlyBlightCount++;
            Blynk.virtualWrite(VPIN_EARLY_BLIGHT, earlyBlightCount);
        }
        else if (strstr(disease, "late_blight") || strstr(disease, "late-blight")) {
            lateBlightCount++;
            Blynk.virtualWrite(VPIN_LATE_BLIGHT, lateBlightCount);
        }
        else if (strstr(disease, "tylcv") || strstr(disease, "curl") || strstr(disease, "yellow")) {
            tylcvCount++;
            Blynk.virtualWrite(VPIN_TYLCV, tylcvCount);
        }
        
        // Send notification for disease detection
        if (strcmp(disease, "healthy") != 0) {
            Blynk.notify(String("Disease detected: ") + disease);
        }
    }
}

void storeDetectionOffline(const char* disease, float confidence) {
    if (!sd_available) return;
    
    StaticJsonDocument<200> doc;
    doc["timestamp"] = millis();
    doc["disease"] = disease;
    doc["confidence"] = confidence;
    
    String jsonString;
    serializeJson(doc, jsonString);
    offline_detections.push_back(jsonString);
    
    // Save to SD card
    File file = SD_MMC.open(OFFLINE_FILE, FILE_APPEND);
    if (file) {
        file.println(jsonString);
        file.close();
    }
}

void syncOfflineDetections() {
    if (!sd_available || !Blynk.connected()) return;
    
    File file = SD_MMC.open(OFFLINE_FILE, FILE_READ);
    if (!file) return;
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (line.length() > 0) {
            DynamicJsonDocument doc(200);
            deserializeJson(doc, line);
            
            const char* disease = doc["disease"];
            float confidence = doc["confidence"];
            
            sendDetectionToBlynk(disease, confidence);
        }
    }
    file.close();
    
    // Clear offline file after sync
    SD_MMC.remove(OFFLINE_FILE);
}

void setup() {
  Serial.begin(115200);
  
  // Initialize camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_240X240;  // Changed from FRAMESIZE_96X96
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Load TFLite model
  model = tflite::GetModel(tomato_model_tflite);
  static tflite::MicroErrorReporter micro_error_reporter;
  tflite::MicroMutableOpResolver<10> micro_op_resolver;
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddMaxPool2D();
  micro_op_resolver.AddFullyConnected();
  micro_op_resolver.AddSoftmax();
  
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize, &micro_error_reporter);
  interpreter = &static_interpreter;
  interpreter->AllocateTensors();
  input = interpreter->input(0);

  // Initialize WiFi and Blynk
  Blynk.begin(auth, ssid, password);

  // Initialize SD card
  if(!SD_MMC.begin()) {
      Serial.println("SD Card Mount Failed");
  } else {
      sd_available = true;
      Serial.println("SD Card Mounted");
  }
}

void loop() {
  Blynk.run();
  
  // Get camera frame
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Process image and run inference
  if(fb) {
    // Resize from 240x240 to model input size
    // Process image and run inference
    bool disease_detected = false; // Placeholder for actual detection logic
    const char* disease_label = "Tomato Disease"; // Placeholder for actual label
    float confidence = 0.9; // Placeholder for actual confidence

    if(fb && disease_detected) {
        if (Blynk.connected()) {
            sendDetectionToBlynk(disease_label, confidence);
            syncOfflineDetections();  // Syncs stored data when Blynk reconnects
        } else {
            storeDetectionOffline(disease_label, confidence);  // Saves to SD card
        }
    }
  }
  
  esp_camera_fb_return(fb);
  delay(5000); // Check every 5 seconds
}