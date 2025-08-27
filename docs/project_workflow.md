# Complete Workflow: Tomato Disease Detection System

## Phase 1: Dataset Preparation

1. **Organize Raw Dataset**
   - Create dataset folders in `raw_dataset/`
   - Populate with 500 images per class total of 2000
   - Classes:
     - healthy_leaf
     - early_blight_leaf
     - late_blight_leaf
     - septoria leaf spot leaf

2. **Preprocess Images**
   ```bash
   python model/preprocess.py
   ```
   - Resizes images to 96x96px
   - Applies CLAHE enhancement
   - Normalizes pixel values
   - Outputs processed images to `processed_dataset/`

3. **Augment Dataset**
   ```bash
   python model/augment_dataset.py
   ```
   - Creates 10 variants of each image
   - Applies rotations, flips, brightness changes, etc.
   - Increases dataset size 10×
   - Outputs to `augmented_dataset/`

4. **Prepare Training Dataset**
   ```bash
   python model/prepare_dataset.py
   ```
   - Organizes images into train/validation sets (80%/20% split)
   - Creates required directory structure
   - Outputs to `data/train` and `data/validation`

## Phase 2: Model Training and Conversion

1. **Train CNN Model**
   ```bash
   python model/tomato_cnn.py
   ```
   - Uses transfer learning with MobileNetV2
   - Two-phase training:
     - Phase 1: Train only top layers (10 epochs)
     - Phase 2: Fine-tune last 20 layers (5 epochs)
   - Achieves ~90% validation accuracy

2. **Convert Model to TFLite**
   - Automatically happens at end of training
   - Optimizes with INT8 quantization
   - Reduces model size by ~75%
   - Outputs to `esp32/model/tomato_model.tflite`

3. **Generate C++ Model Files**
   ```bash
   xxd -i tomato_model.tflite > tomato_model_tflite.cpp
   ```
   - Creates C++ header/implementation files
   - Places them in Arduino project folder

## Phase 3: ESP32-CAM Setup

1. **Hardware Assembly**
   - Connect ESP32-CAM to development board
   - Add ILI9341 TFT display (optional)
   - Insert SD card for offline storage
   - Ensure adequate power supply (5V)

2. **Arduino IDE Setup**
   - Install ESP32 board support
   - Install required libraries:
     - TensorFlow Lite for ESP32
     - ArduinoJson
     - ESP32 Camera
     - Blynk (if using Blynk version)
     - Adafruit GFX & ILI9341 (if using display)

3. **Prepare Arduino Project**
   - Choose monitoring system:
     - `tomato_detection_blynk.ino` (for mobile app monitoring)
     - `tomato_detection.ino` (for Flask server monitoring)
   - Add model files to Arduino project:
     - `tomato_model_tflite.h`
     - `tomato_model_tflite.cpp`
   - Configure WiFi credentials and server/auth details

4. **Upload to ESP32-CAM**
   - Connect ESP32-CAM via development board
   - Enter programming mode (GPIO0 to GND or PROG button)
   - Upload code using Arduino IDE
   - Reset device after uploading

## Phase 4: Monitoring System Setup

### Option A: Blynk Mobile App
1. **App Setup**
   - Install Blynk IoT app
   - Create account and project
   - Get authentication token
   - Configure widgets according to `docs/blynk_setup.md`

2. **ESP32 Configuration**
   - Update Blynk auth token in code
   - Ensure WiFi credentials are correct
   - Upload code to ESP32-CAM

### Option B: Flask Server
1. **Server Setup**
   ```bash
   pip install -r server/requirements.txt
   ```
   - Configure Pushbullet API key in `server/notification.py`
   - Start server:
   ```bash
   python server/app.py
   ```
   - Access web interface at `http://server-ip:5000`

## Phase 5: Operation and Monitoring

1. **System Operation**
   - ESP32-CAM captures images every 5 seconds
   - TFLite model performs inference
   - Results shown on TFT display (if installed)
   - Data transmitted to Blynk or Flask server
   - Notifications sent when diseases detected

2. **Offline Operation**
   - When WiFi unavailable, detections stored on SD card
   - Data synchronized when connection restored
   - Local display continues to show results

3. **Monitoring**
   - Check mobile app or web interface for detection history
   - Review statistics on disease prevalence
   - Observe alert notifications
   - Take action based on detected diseases

## Phase 6: Maintenance and Updates

1. **System Maintenance**
   - Periodically check SD card storage
   - Monitor battery level (if battery-powered)
   - Clean camera lens for optimal image quality
   - Verify WiFi connectivity

2. **Model Updates**
   - Collect additional training data if needed
   - Retrain model with new data
   - Convert to TFLite and update ESP32-CAM
   - Test new model performance

