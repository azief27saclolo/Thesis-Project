// Add these includes
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include "SD_MMC.h"
#include <ArduinoJson.h>
#include "tomato_model_tflite.h"  // Missing model header include
#include <esp_camera.h>  // More specific camera include
#include "power_monitor.h"  // Include power monitor header
#include "software_power_monitor.h"  // Include software power monitor header

// Add TFT display libraries
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// Blynk authentication token - get this from Blynk app
char auth[] = "YourBlynkAuthToken";
char ssid[] = "YourWiFiName";
char password[] = "YourWiFiPassword";

#define OFFLINE_FILE "/offline_detections.json"
bool sd_available = false;

// Virtual pins for Blynk
#define VPIN_DISEASE     V0
#define VPIN_CONFIDENCE  V1
#define VPIN_TIMESTAMP   V2
#define VPIN_ALARM       V3  // Changed from VPIN_IMAGE
#define VPIN_SOUND       V8  // Added new pin for sound alerts
#define VPIN_HEALTHY     V4
#define VPIN_EARLY_BLIGHT V5
#define VPIN_LATE_BLIGHT V6
#define VPIN_TYLCV       V7

// Add spray control pin and virtual pin for manual control
#define SPRAY_RELAY_PIN  4     // GPIO pin connected to relay for sprayer
#define VPIN_MANUAL_SPRAY V9   // Virtual pin for manual spray control

// Disease counters
int healthyCount = 0;
int earlyBlightCount = 0;
int lateBlightCount = 0;
int tylcvCount = 0;

// Add spray settings
bool autoSprayEnabled = true;  // Enable automatic spraying by default
unsigned long lastSprayTime = 0;
const unsigned long SPRAY_COOLDOWN = 3600000; // 1 hour cooldown between auto sprays

// Camera pins - missing definitions
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

// TFT display pins for ESP32-CAM
// Note: Choose GPIO pins that don't conflict with camera
#define TFT_CS   14  // Chip select
#define TFT_DC   15  // Data/Command
#define TFT_MOSI 13  // SPI MOSI
#define TFT_CLK  12  // SPI Clock
#define TFT_RST  2   // Reset
#define TFT_MISO -1  // Not connected

// Initialize TFT display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

// Define colors
#define TFT_BACKGROUND ILI9341_BLACK
#define TFT_TEXT       ILI9341_WHITE
#define TFT_HEALTHY    ILI9341_GREEN
#define TFT_DISEASE    ILI9341_RED
#define TFT_BORDER     ILI9341_BLUE

// Missing TFLite namespace and variables
namespace {
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* input = nullptr;
  constexpr int kTensorArenaSize = 150000;
  uint8_t tensor_arena[kTensorArenaSize];
}

// Missing vector declaration for offline storage
std::vector<String> offline_detections;

// Initialize power monitor
PowerMonitor powerMonitor;

// Initialize software power monitor
SoftwarePowerMonitor softwarePowerMonitor;

void setup() {
  Serial.begin(115200);
  
  // Initialize spray control pin
  pinMode(SPRAY_RELAY_PIN, OUTPUT);
  digitalWrite(SPRAY_RELAY_PIN, LOW); // Make sure sprayer is off initially
  
  // Initialize TFT display
  tft.begin();
  tft.setRotation(3); // Landscape mode
  tft.fillScreen(TFT_BACKGROUND);
  tft.setTextSize(2);
  tft.setTextColor(TFT_TEXT);
  tft.setCursor(10, 10);
  tft.println("Tomato Disease");
  tft.println("Detection System");
  tft.println("Initializing...");
  
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

  // Initialize SD card
  if(!SD_MMC.begin()) {
    Serial.println("SD Card Mount Failed");
  } else {
    sd_available = true;
    Serial.println("SD Card Mounted");
  }
  
  // Initialize Blynk
  Blynk.begin(auth, ssid, password);
  
  // Initialize power monitor
  powerMonitor.begin();
  
  // Initialize software power monitor
  softwarePowerMonitor.begin();
  
  // Update display after initialization
  tft.fillScreen(TFT_BACKGROUND);
  tft.setCursor(10, 10);
  tft.println("Ready to detect");
  tft.println("diseases");
}

// Add function to activate sprayer
void activateSprayer(int durationMs = 5000) {
  Serial.println("Activating plant sprayer");
  
  // Update display to show spraying status
  tft.fillRect(0, 200, tft.width(), 40, TFT_BACKGROUND);
  tft.setCursor(10, 200);
  tft.setTextColor(TFT_TEXT);
  tft.setTextSize(2);
  tft.println("SPRAYING...");
  
  // Activate the relay/sprayer
  digitalWrite(SPRAY_RELAY_PIN, HIGH);
  
  // Keep the sprayer on for specified duration
  delay(durationMs);
  
  // Turn off the sprayer
  digitalWrite(SPRAY_RELAY_PIN, LOW);
  
  // Update display
  tft.fillRect(0, 200, tft.width(), 40, TFT_BACKGROUND);
  tft.setCursor(10, 200);
  tft.setTextSize(2);
  tft.println("Spray complete");
  
  // Record the time of spraying
  lastSprayTime = millis();
}

// Add Blynk handler for manual spray button
BLYNK_WRITE(VPIN_MANUAL_SPRAY) {
  int value = param.asInt();
  if (value == 1) {
    // Button pressed - activate sprayer
    activateSprayer();
    Blynk.virtualWrite(VPIN_MANUAL_SPRAY, 0); // Reset button state
  }
}

// Add toggle for automatic spraying
BLYNK_WRITE(VPIN_AUTO_SPRAY) {
  autoSprayEnabled = param.asInt();
  Serial.print("Auto-spray ");
  Serial.println(autoSprayEnabled ? "enabled" : "disabled");
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
      
      // Send to Blynk
      Blynk.virtualWrite(VPIN_DISEASE, doc["disease"].as<String>());
      Blynk.virtualWrite(VPIN_CONFIDENCE, doc["confidence"].as<float>());
      Blynk.virtualWrite(VPIN_TIMESTAMP, doc["timestamp"].as<long>());
      
      // Send notification if disease
      if (!doc["disease"].as<String>().equals("healthy")) {
        Blynk.notify("Disease detected: " + doc["disease"].as<String>());
      }
      
      delay(100); // Small delay between sends
    }
  }
  file.close();
  
  // Clear file after sync
  SD_MMC.remove(OFFLINE_FILE);
}

void updateDiseaseCount(const char* disease) {
  // Increment appropriate counter
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
}

// Add function to display detection results on TFT
void updateDisplay(const char* disease, float confidence) {
  // Clear the display area for results
  tft.fillRect(0, 80, tft.width(), 160, TFT_BACKGROUND);
  
  // Display the disease name
  tft.setCursor(10, 80);
  tft.setTextSize(2);
  
  // Change color based on detection
  if (strstr(disease, "healthy")) {
    tft.setTextColor(TFT_HEALTHY);
  } else {
    tft.setTextColor(TFT_DISEASE);
  }
  
  tft.println(disease);
  
  // Display confidence
  tft.setCursor(10, 120);
  tft.setTextColor(TFT_TEXT);
  tft.print("Confidence: ");
  tft.print((int)(confidence * 100));
  tft.println("%");
  
  // Display connection status
  tft.setCursor(10, 160);
  tft.setTextSize(1);
  if (Blynk.connected()) {
    tft.println("Blynk: Connected");
  } else {
    tft.println("Blynk: Disconnected");
    tft.println("Saving data locally");
  }
  
  // Add battery status display
  tft.setCursor(10, 200);
  int batteryPercent = powerMonitor.getBatteryPercentage();
  float voltage = powerMonitor.getCurrentVoltage();
  
  // Set color based on battery level
  if (batteryPercent > 70) {
    tft.setTextColor(TFT_HEALTHY);
  } else if (batteryPercent > 30) {
    tft.setTextColor(ILI9341_YELLOW);
  } else {
    tft.setTextColor(TFT_DISEASE);
  }
  
  tft.print("Battery: ");
  tft.print(batteryPercent);
  tft.print("% (");
  tft.print(voltage, 2);
  tft.print("V)");
  
  // Add ML power consumption to display
  tft.setCursor(10, 220);
  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(1);
  tft.print("ML avg time: ");
  tft.print(softwarePowerMonitor.getAverageInferenceTime(), 1);
  tft.print("ms");
  
  tft.setCursor(10, 230);
  tft.print("Est. power: ");
  tft.print(softwarePowerMonitor.getTotalPowerUsage(), 2);
  tft.print("mAh");
}

void sendDetection(const char* disease, float confidence) {
  // Update the TFT display with detection results
  updateDisplay(disease, confidence);
  
  if (Blynk.connected()) {
    Blynk.virtualWrite(VPIN_DISEASE, disease);
    Blynk.virtualWrite(VPIN_CONFIDENCE, confidence);
    Blynk.virtualWrite(VPIN_TIMESTAMP, millis());
    
    // Update disease counts
    updateDiseaseCount(disease);
    
    if (strcmp(disease, "healthy") != 0) {
      // Trigger alarm and sound for disease detection
      Blynk.virtualWrite(VPIN_ALARM, 1);  // Turn on alarm
      Blynk.virtualWrite(VPIN_SOUND, 1);  // Play sound
      Blynk.notify(String("Disease detected: ") + disease);
      
      // Handle automatic spraying for disease
      if (autoSprayEnabled && (millis() - lastSprayTime > SPRAY_COOLDOWN)) {
        activateSprayer();
      }
    } else {
      // Turn off alarm if healthy
      Blynk.virtualWrite(VPIN_ALARM, 0);
      Blynk.virtualWrite(VPIN_SOUND, 0);
    }
    
    syncOfflineDetections();
  } else {
    storeDetectionOffline(disease, confidence);
  }
}

void loop() {
  Blynk.run(); // Handle Blynk connection
  
  // Check battery status - also updates the internal voltage reading
  powerMonitor.checkPowerConsumption();
  
  // Get camera frame
  softwarePowerMonitor.startCamera();
  camera_fb_t* fb = esp_camera_fb_get();
  softwarePowerMonitor.endCamera();
  
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Process image and run inference
  bool disease_detected = false;
  const char* disease_label = "healthy_leaf"; // Default
  float confidence = 0.0;
  
  if (fb) {
    // Mark start of inference for power measurement
    powerMonitor.startInference();
    softwarePowerMonitor.startInference();
    
    // Resize image to model input size (96x96)
    uint8_t* resized_input = (uint8_t*)input->data.uint8;
    
    // Process frame and resize to model input size
    // TODO: Add image preprocessing code here
    
    // Run inference
    interpreter->Invoke();
    
    // Mark end of inference
    powerMonitor.endInference();
    softwarePowerMonitor.endInference();
    
    // Find class with highest confidence
    TfLiteTensor* output = interpreter->output(0);
    float max_score = 0.0;
    int max_index = 0;
    
    // Process results and determine disease
    // TODO: Add result processing code here
    
    // Example of setting variables based on detection
    disease_detected = true;
    disease_label = "early_blight_leaf"; // This would come from your model output
    confidence = 0.95; // This would come from your model output
    
    if(disease_detected) {
      sendDetection(disease_label, confidence);
    }
  }
  
  // Return the frame buffer
  esp_camera_fb_return(fb);
  
  // Print power stats every minute
  static unsigned long lastStatPrint = 0;
  if (millis() - lastStatPrint > 60000) {
    softwarePowerMonitor.printPowerStats();
    lastStatPrint = millis();
  }
  
  // Add a delay to avoid processing too many frames
  #ifdef POWER_SAVING_MODE
    delay(1800000);  // 30 minutes in power saving mode
  #else
    delay(300000);   // 5 minutes in normal operation
  #endif
}
