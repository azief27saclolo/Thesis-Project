# Tools and Environment in Tomato Disease Detection Project

## Development Environments

### 1. Arduino IDE (Version 1.8.x/2.x)
- **Purpose**: Programming and uploading code to the ESP32-CAM microcontroller
- **Function**: Provides the development environment, code editor, libraries management, and device programmer
- **Configuration**: Set to ESP32 Wrover Module with Huge APP partition scheme (3MB No OTA/1MB SPIFFS)

### 2. Python Environment (Version 3.7-3.9)
- **Purpose**: Training the machine learning model and data preparation
- **Function**: Executes image preprocessing, augmentation, model training, and TFLite conversion
- **Implementation**: Uses virtual environments (venv/conda) to maintain clean dependency isolation

### 3. Visual Studio Code (VSCode)
- **Purpose**: Primary code editor for Python scripts and Arduino code
- **Function**: Provides syntax highlighting, code completion, and integrated terminal
- **Extensions**: Python, Arduino, IntelliSense

## Machine Learning Framework

### 1. TensorFlow/Keras (TF 2.x)
- **Purpose**: Machine learning framework for model development
- **Function**: Handles neural network architecture, training, and optimization
- **Features Used**: Transfer learning (MobileNetV2), data augmentation, quantization

### 2. TensorFlow Lite
- **Purpose**: Model optimization and conversion for embedded deployment
- **Function**: Reduces model size through quantization and operation optimization
- **Output**: Generates a compact model file (tomato_model.tflite) suitable for microcontrollers

### 3. TensorFlow Lite for Microcontrollers
- **Purpose**: Running inference on ESP32-CAM
- **Function**: Interprets the TFLite model and performs predictions on captured images
- **Features**: Optimized for devices with limited RAM and processing power

## Image Processing Tools

### 1. OpenCV (cv2)
- **Purpose**: Image manipulation and preprocessing
- **Function**: Resizing, color conversion, CLAHE enhancement, augmentation
- **Methods Used**: cv2.resize(), cv2.cvtColor(), CLAHE, conversion between color spaces

### 2. NumPy
- **Purpose**: Efficient numerical operations on image data
- **Function**: Array manipulation, statistical functions for image analysis
- **Applications**: Normalization, data storage, array operations

## Hardware Components

### 1. ESP32-CAM
- **Purpose**: Image capture and machine learning inference
- **Function**: Takes pictures of tomato plants, runs the TensorFlow Lite model
- **Specs**: 4MB Flash, 520KB SRAM, OV2640 2MP camera, WiFi/Bluetooth

### 2. TFT ILI9341 Display
- **Purpose**: Real-time visual feedback on device
- **Function**: Shows detection results, battery status, and system status
- **Interface**: SPI connection to ESP32-CAM

### 3. SD Card
- **Purpose**: Offline storage for detections when connectivity is lost
- **Function**: Saves detection results as JSON for later synchronization
- **Implementation**: Uses SD_MMC library for file operations

### 4. Spray Control System
- **Purpose**: Automated disease management
- **Function**: Activates sprayer when diseases are detected
- **Components**: Relay connected to GPIO4, compatible with 5V/12V spray systems

## IoT and Connectivity

### 1. Blynk IoT Platform
- **Purpose**: Remote monitoring and control through mobile app
- **Function**: Receives detection data, displays statistics, sends notifications
- **Features Used**: Virtual pins, notifications, data visualization, remote control

### 2. WiFi
- **Purpose**: Connectivity for data transmission
- **Function**: Connects ESP32-CAM to local network for Blynk communication
- **Implementation**: Uses ESP32 WiFi library with reconnection handling

## Libraries and Dependencies

### For ESP32:
- **BlynkSimpleEsp32**: Blynk connectivity
- **TensorFlowLite_ESP32**: TF inference
- **ArduinoJson**: Data serialization
- **SD_MMC**: Storage access
- **Adafruit_GFX/ILI9341**: Display control

### For Python (Training):
- **TensorFlow/Keras**: Model development
- **OpenCV-Python**: Image processing
- **NumPy**: Numerical operations
- **scikit-learn**: Dataset splitting
- **Matplotlib**: Visualization

## Data Processing Pipeline

1. **Collection**: Raw images stored in raw_dataset/
2. **Preprocessing**: Enhanced with CLAHE, resizing (preprocess.py)
3. **Augmentation**: Generated variations of original images (augment_dataset.py)
4. **Preparation**: Split into train/validation sets (prepare_dataset.py)
5. **Training**: MobileNetV2 fine-tuning (tomato_cnn.py)
6. **Optimization**: Quantization and conversion to TFLite
7. **Deployment**: Conversion to C++ arrays for ESP32-CAM

## System Architecture

### Edge Computing Layer (ESP32-CAM)
- Captures images every 5 minutes
- Runs TensorFlow Lite inference
- Sends results to cloud via Blynk
- Local display for immediate feedback
- Offline capability with SD card storage

### Cloud Layer (Blynk)
- Receives detection results
- Sends push notifications for diseases
- Provides mobile interface for monitoring
- Stores historical data
- Enables remote sprayer control

This architecture combines the benefits of edge computing (low latency, offline operation) with cloud capabilities (remote access, data aggregation, notifications).
