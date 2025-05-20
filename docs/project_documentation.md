# Tomato Disease Detection System Using ESP32-CAM
## System Documentation

### 1. Project Overview
This system implements a real-time tomato disease detection system using:
- ESP32-CAM for image capture
- MobileNetV2 (lightweight CNN) for disease detection
- Web interface for monitoring
- Push notifications for alerts

### 2. System Components

#### 2.1 AI Model (MobileNetV2)
- Pre-trained on ImageNet
- Modified for tomato diseases and fruit health (5 classes):
  - Healthy Fruit
  - Healthy Leaf
  - Early Blight
  - Late Blight
  - Tomato Yellow Leaf Curl Virus
- Optimized to 35% of original size (α=0.35)
- Input size: 96x96x3 pixels

#### 2.2 Training Process
1. Dataset Preparation:
   ```
   raw_dataset/
   ├── healthy/           (150 images)
   ├── early_blight/      (150 images)
   ├── late_blight/       (150 images)
   └── tylcv/            (150 images)
   ```

2. Training Pipeline:
   - Preprocess images (resize, normalize)
   - Augment dataset (increase samples)
   - Train MobileNetV2 (transfer learning)
   - Convert to TFLite format

3. Two-Phase Training:
   - Phase 1: Train only new top layers
   - Phase 2: Fine-tune last 20 layers
   - Uses Adam optimizer (learning rate: 1e-5)

#### 2.3 ESP32-CAM Implementation
- Captures images in real-time
- Runs TFLite model inference
- Connects to WiFi for data transmission
- Stores offline detections on SD card
- Syncs data when online

#### 2.4 Web Monitoring System
- Flask server backend
- SQLite database for detection history
- Real-time web interface
- Push notifications via Pushbullet
- Mobile-friendly design

### 3. Technical Details

#### 3.1 Model Architecture
```
MobileNetV2 (α=0.35)
├── Input (96x96x3)
├── Base layers (ImageNet pre-trained)
├── Global Average Pooling
├── Dense layer (32 neurons, ReLU)
├── Dropout (0.2)
└── Output layer (4 classes, Softmax)
```

#### 3.2 Data Processing
1. Image Preprocessing:
   - Resize to 96x96
   - RGB conversion
   - Normalize pixel values
   - CLAHE enhancement

2. Data Augmentation:
   - Random rotation
   - Random translation
   - Random zoom
   - Horizontal flip
   - Brightness adjustment

#### 3.3 System Architecture
```
ESP32-CAM
   ↓
   ↓ (WiFi/Local Storage)
   ↓
Flask Server
   ↓
   ↓ (Web Interface/Database)
   ↓
User Devices
```

### 4. Implementation Guide

#### 4.1 Initial Setup
1. Install requirements:
   ```bash
   pip install -r model/requirements.txt
   pip install -r server/requirements.txt
   ```

2. Prepare dataset structure:
   ```
   raw_dataset/
   ├── healthy/
   ├── early_blight/
   ├── late_blight/
   └── tylcv/
   ```

#### 4.2 Training Pipeline
Run in order:
```bash
python model/preprocess.py
python model/augment_dataset.py
python model/prepare_dataset.py
python model/tomato_cnn.py
```

#### 4.3 Server Setup
1. Configure Pushbullet API key
2. Start server:
   ```bash
   python server/app.py
   ```

#### 4.4 ESP32-CAM Setup
1. Install Arduino libraries:
   - TensorFlow Lite
   - ESP32 Camera
   - ArduinoJson

2. Upload code:
   - tomato_detection.ino
   - tomato_model_tflite.h
   - tomato_model_tflite.cpp

### 5. Key Features
- Real-time disease detection
- Offline operation capability
- Automatic data synchronization
- Push notifications
- Mobile monitoring interface
- Detection history tracking
- Low resource requirements

### 6. Performance Metrics
- Target accuracy: 90%+
- Real-time inference
- Minimal latency
- Efficient resource usage
- Reliable disease detection

### 7. Future Improvements
- Additional disease classes
- Enhanced image preprocessing
- Model optimization
- Battery optimization
- Extended offline capabilities
