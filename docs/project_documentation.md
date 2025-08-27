# Tomato Disease Detection System Using ESP32-CAM
## System Documentation

### 1. Project Overview
This system implements a real-time tomato disease detection system using:
- ESP32-CAM for image capture
- Lightweight CNN for disease detection
- Google Cloud Functions for processing and analyzing the uploaded image captured by ESP32-CAM
- Blink IoT App interface for monitoring 
- Push notifications for alerts

### 2. System Components

#### 2.1 AI Model (MobileNetV2)
- Pre-trained on ImageNet
- Modified for tomato diseases and fruit health (4 classes):
  - Healthy Leaf
  - Early Blight
  - Late Blight
  - Septoria Leaf Spot
- Optimized to 35% of original size (α=0.35)
- Input size: 96x96x3 pixels

#### 2.2 Training Process
1. Dataset Preparation:
   ```
   raw_dataset/
   ├── healthy/           (500 images)
   ├── early_blight/      (500 images)
   ├── late_blight/       (500 images)
   └── septoria_leaf/     (500 images)
   ```

2. Training Pipeline:
   - Preprocess images (resize, normalize)
   - Augment dataset (increase samples)
   - Train Lightweight CNN (transfer learning)
   - Convert to TFLite format

3. Two-Phase Training:
   - Phase 1: Train only new top layers
   - Phase 2: Fine-tune last 20 layers
   - Uses Adam optimizer (learning rate: 1e-5)

#### 2.3 ESP32-CAM Implementation
- Connects to WiFi for data transmission
- Captures images in real-time
- Upload the image to Google Cloud Function for inference and process
- Stores results to Firebase such as logs and time and date.

#### 2.4 Web Monitoring System
- Google Cloud Functions as server
- Firebase for result storing
- Real-time web interface
- Push notifications via Blynk Iot App
- Mobile-friendly design
- Monitor device via Blynk Iot App

### 3. Technical Details

#### 3.1 Model Architecture
```
Lightweight CNN (α=0.35)
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
   ↓ Captures Image
   ↓
Google Cloud Function Server
   ↓
   ↓ 
   ↓
Firebase
   ↓
   ↓ Stores detection logs
   ↓
Blynk Iot App
   ↓
   ↓ View results via dashboard
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
   └── septoria_leaf
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
- Online operation capability
- Automatic data synchronization
- Push notifications
- Mobile monitoring interface
- Detection history tracking


### 6. Performance Metrics
- Target accuracy: 90%+
- Real-time inference
- Minimal latency
- Reliable disease detection

### 7. Future Improvements
- Additional disease classes
- Enhanced image preprocessing
- Model optimization
- Battery optimization
- Extended offline capabilities

