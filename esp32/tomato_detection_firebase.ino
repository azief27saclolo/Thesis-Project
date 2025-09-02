#include "esp_camera.h"
#include "Arduino.h"
#include "WiFi.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "FirebaseESP32.h"
#include <BlynkSimpleEsp32.h>

// Firebase credentials
#define FIREBASE_HOST "your-project-id.firebaseio.com"
#define FIREBASE_AUTH "your-firebase-database-secret"
#define FIREBASE_STORAGE_BUCKET "your-project-id.appspot.com"

// WiFi credentials
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"

// Blynk credentials
#define BLYNK_AUTH "your-blynk-auth-token"
#define BLYNK_PRINT Serial

// Camera config for AI Thinker ESP32-CAM
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

// GPIO for flash control
#define FLASH_LED_PIN 4

// Capture interval in minutes
unsigned long capturePeriod = 60 * 60 * 1000; // 1 hour by default
unsigned long lastCapture = 0;
bool captureEnabled = true;

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Functions
void initCamera();
void captureAndUpload();
void uploadToFirebase(uint8_t *imageData, size_t imageSize);
void updateStatusDatabase(String status, String message);

// Blynk virtual pins
#define VPIN_CAPTURE_INTERVAL 1
#define VPIN_CAPTURE_TOGGLE 2
#define VPIN_CAPTURE_NOW 3
#define VPIN_LATEST_RESULT 4
#define VPIN_BATTERY_LEVEL 5

// For battery monitoring
#define BATTERY_PIN 33
int batteryLevel = 0;

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout detector
  
  Serial.begin(115200);
  Serial.println("ESP32-CAM Tomato Disease Detection - Cloud Version");

  // Initialize flash LED
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi with IP: ");
  Serial.println(WiFi.localIP());

  // Initialize camera
  initCamera();
  
  // Initialize Firebase
  config.host = FIREBASE_HOST;
  config.api_key = FIREBASE_AUTH;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  // Set database read timeout to 1 minute
  Firebase.setReadTimeout(fbdo, 1000 * 60);
  // Set database write size limit
  Firebase.setwriteSizeLimit(fbdo, "tiny");
  
  // Initialize Blynk
  Blynk.begin(BLYNK_AUTH, WIFI_SSID, WIFI_PASSWORD);
  
  // Register device with Firebase
  String deviceId = "ESP32CAM_" + String(ESP.getEfuseMac(), HEX);
  Firebase.setString(fbdo, "/devices/" + deviceId + "/status", "online");
  Firebase.setString(fbdo, "/devices/" + deviceId + "/last_seen", Firebase.getCurrentTimestamp());
  
  updateStatusDatabase("initialized", "ESP32-CAM is ready");
  
  // Initial sync with Blynk
  Blynk.virtualWrite(VPIN_CAPTURE_INTERVAL, capturePeriod / 60000); // minutes
  Blynk.virtualWrite(VPIN_CAPTURE_TOGGLE, captureEnabled ? 1 : 0);
  
  // Take initial image
  captureAndUpload();
}

void loop() {
  Blynk.run();
  
  // Read battery level
  batteryLevel = map(analogRead(BATTERY_PIN), 0, 4095, 0, 100);
  Blynk.virtualWrite(VPIN_BATTERY_LEVEL, batteryLevel);
  
  // Check if it's time to capture
  unsigned long currentMillis = millis();
  if (captureEnabled && (currentMillis - lastCapture > capturePeriod)) {
    captureAndUpload();
    lastCapture = currentMillis;
  }
  
  // Check Firebase for commands
  if (Firebase.getString(fbdo, "/devices/ESP32CAM_" + String(ESP.getEfuseMac(), HEX) + "/command")) {
    String command = fbdo.stringData();
    
    if (command == "capture_now") {
      captureAndUpload();
      Firebase.setString(fbdo, "/devices/ESP32CAM_" + String(ESP.getEfuseMac(), HEX) + "/command", "");
    }
    else if (command.startsWith("set_interval:")) {
      int minutes = command.substring(13).toInt();
      capturePeriod = minutes * 60 * 1000;
      Blynk.virtualWrite(VPIN_CAPTURE_INTERVAL, minutes);
      Firebase.setString(fbdo, "/devices/ESP32CAM_" + String(ESP.getEfuseMac(), HEX) + "/command", "");
    }
  }
  
  // Check for latest analysis result
  if (Firebase.getString(fbdo, "/devices/ESP32CAM_" + String(ESP.getEfuseMac(), HEX) + "/latest_result")) {
    String result = fbdo.stringData();
    Blynk.virtualWrite(VPIN_LATEST_RESULT, result);
  }
  
  delay(1000);
}

void initCamera() {
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
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Select lower framesize for higher image quality (FRAMESIZE_UXGA, FRAMESIZE_SXGA, FRAMESIZE_XGA, FRAMESIZE_SVGA)
  config.frame_size = FRAMESIZE_SVGA; // 800x600 resolution
  config.jpeg_quality = 10; // 0-63, lower number means higher quality
  config.fb_count = 2;
  
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    updateStatusDatabase("error", "Camera initialization failed");
    delay(1000);
    ESP.restart();
  }
  
  sensor_t * s = esp_camera_sensor_get();
  // Initial sensor settings
  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, 0);       // -2 to 2
  s->set_saturation(s, 0);     // -2 to 2
  s->set_special_effect(s, 0); // 0 = No Effect, 1 = Negative, 2 = Grayscale
  s->set_whitebal(s, 1);       // 0 = disable, 1 = enable
  s->set_awb_gain(s, 1);       // 0 = disable, 1 = enable
  s->set_wb_mode(s, 0);        // 0 auto
  s->set_exposure_ctrl(s, 1);  // 0 = disable, 1 = enable
  s->set_gain_ctrl(s, 1);      // 0 = disable, 1 = enable
  s->set_aec2(s, 0);           // 0 = disable, 1 = enable
  s->set_ae_level(s, 0);       // -2 to 2
  s->set_aec_value(s, 300);    // 0 to 1200
  s->set_hmirror(s, 0);        // 0 = disable, 1 = enable
  s->set_vflip(s, 0);          // 0 = disable, 1 = enable
  s->set_agc_gain(s, 0);       // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
  s->set_bpc(s, 0);            // 0 = disable, 1 = enable
  s->set_wpc(s, 1);            // 0 = disable, 1 = enable
  s->set_raw_gma(s, 1);        // 0 = disable, 1 = enable
  s->set_lenc(s, 1);           // 0 = disable, 1 = enable
  s->set_dcw(s, 1);            // 0 = disable, 1 = enable
  s->set_colorbar(s, 0);       // 0 = disable, 1 = enable
}

void captureAndUpload() {
  updateStatusDatabase("capturing", "Taking image...");
  
  // Turn on flash
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(200); // Give time for flash to affect scene
  
  // Capture frame
  camera_fb_t *fb = esp_camera_fb_get();
  
  // Turn off flash
  digitalWrite(FLASH_LED_PIN, LOW);
  
  if (!fb) {
    Serial.println("Camera capture failed");
    updateStatusDatabase("error", "Camera capture failed");
    return;
  }
  
  updateStatusDatabase("uploading", "Uploading image to Firebase...");
  
  // Upload image to Firebase
  uploadToFirebase(fb->buf, fb->len);
  
  // Return the frame buffer to be reused
  esp_camera_fb_return(fb);
}

void uploadToFirebase(uint8_t *imageData, size_t imageSize) {
  // Create a unique filename based on timestamp
  String deviceId = "ESP32CAM_" + String(ESP.getEfuseMac(), HEX);
  String fileName = deviceId + "_" + String(millis()) + ".jpg";
  
  Serial.println("Uploading image to Firebase Storage: " + fileName);
  
  // Upload to Firebase Storage
  if (Firebase.pushFile(fbdo, FIREBASE_STORAGE_BUCKET, fileName.c_str(), imageData, imageSize)) {
    Serial.println("Upload successful");
    
    // Add database entry
    FirebaseJson json;
    json.set("device_id", deviceId);
    json.set("timestamp", Firebase.getCurrentTimestamp());
    json.set("status", "pending_analysis");
    json.set("image_path", fileName);
    
    if (Firebase.pushJSON(fbdo, "/images", json)) {
      Serial.println("Database entry created");
      updateStatusDatabase("idle", "Image uploaded successfully");
    } else {
      Serial.println("Database entry failed: " + fbdo.errorReason());
      updateStatusDatabase("error", "Database update failed: " + fbdo.errorReason());
    }
  } else {
    Serial.println("Upload failed: " + fbdo.errorReason());
    updateStatusDatabase("error", "Upload failed: " + fbdo.errorReason());
  }
}

void updateStatusDatabase(String status, String message) {
  FirebaseJson json;
  json.set("status", status);
  json.set("message", message);
  json.set("timestamp", Firebase.getCurrentTimestamp());
  
  String deviceId = "ESP32CAM_" + String(ESP.getEfuseMac(), HEX);
  Firebase.updateNode(fbdo, "/devices/" + deviceId, json);
}

// Blynk handlers
BLYNK_WRITE(VPIN_CAPTURE_INTERVAL) {
  int minutes = param.asInt();
  capturePeriod = minutes * 60 * 1000;
  Serial.println("Capture interval set to " + String(minutes) + " minutes");
}

BLYNK_WRITE(VPIN_CAPTURE_TOGGLE) {
  captureEnabled = param.asInt();
  Serial.println("Capture " + String(captureEnabled ? "enabled" : "disabled"));
}

BLYNK_WRITE(VPIN_CAPTURE_NOW) {
  if (param.asInt() == 1) {
    Serial.println("Manual capture triggered");
    captureAndUpload();
  }
}
