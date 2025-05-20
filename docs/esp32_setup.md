# ESP32-CAM-MB Setup Guide

## Hardware Required
- ESP32-CAM module
- ESP32-CAM-MB development board (for programming without FTDI)
- Micro USB cable

## Using ESP32-CAM-MB Development Board

### 1. Programming Setup
1. Insert ESP32-CAM module into the development board socket
2. Connect development board to computer via USB cable
3. In Arduino IDE, go to Tools > Board > ESP32 Arduino
4. Select **"AI Thinker ESP32-CAM"** or **"ESP32 Wrover Module"**
5. Configure these settings:
   - Flash Frequency: "80MHz"
   - Upload Speed: "115200"
   - Flash Mode: "QIO" 
   - Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
   - Port: Select the COM port that appears when you connect the board

### 2. Programming Process
1. Some boards have a "PROG" button - press and hold while pressing "RESET" button once
2. Other boards automatically enter programming mode when uploading
3. Click "Upload" in Arduino IDE
4. Wait for "Done uploading" message
5. Press "RESET" button on development board to start your program

### 3. Standalone Usage
1. After successful upload, you can:
   - Keep using the development board (easiest option)
   - OR remove ESP32-CAM from development board for standalone use
   - If used standalone, power ESP32-CAM with 5V and GND pins

## Arduino IDE Board Selection
1. In Arduino IDE, go to Tools > Board > ESP32 Arduino
2. Select **"AI Thinker ESP32-CAM"** or **"ESP32 Wrover Module"**
3. Configure these settings:
   - Flash Frequency: "80MHz"
   - Upload Speed: "115200"
   - Flash Mode: "QIO"
   - Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"

## Connection Diagram
```
ESP32-CAM    |    FTDI Adapter
--------------------------------
5V/3.3V      |    VCC (3.3V recommended)
GND          |    GND
U0R (GPIO3)  |    TX
U0T (GPIO1)  |    RX
GPIO0        |    GND (only during programming)
```

## Complete Workflow

### 1. Train the Model (on PC)
```bash
# Set up Python environment
pip install -r model/requirements.txt

# Process and train on your dataset
python model/preprocess.py
python model/augment_dataset.py
python model/prepare_dataset.py
python model/tomato_cnn.py
```

### 2. Arduino Setup and Upload
1. Install required libraries in Arduino IDE:
   - ESP32 board support
   - TensorFlow Lite for ESP32 (`Arduino_TensorFlowLite`)
   - ArduinoJson (version 6.x)
   - ESP32 Camera (included with ESP32 board package)

2. Prepare ESP32-CAM for upload:
   - Connect GPIO0 to GND
   - Press reset button
   - Connect FTDI adapter according to diagram

3. Upload code:
   - Open `tomato_detection.ino` in Arduino IDE
   - Update WiFi credentials and server IP
   - Select correct port 
   - Click "Upload" button
   - When "Connecting..." appears, release reset button
   - Once uploaded, disconnect GPIO0 from GND
   - Press reset again to run program

### 3. Server Setup (on PC or Raspberry Pi)
```bash
# Install server requirements
pip install -r server/requirements.txt

# Start the Flask server
python server/app.py
```

### 4. Mobile Interface
- Open web browser on any device connected to same WiFi
- Navigate to: `http://server-ip:5000`
- Click "Activate Device" to start monitoring

### 5. Troubleshooting
- If upload fails, try different TX/RX connections
- For "Failed to connect" errors, press reset at the beginning of upload
- If ESP32-CAM restarts repeatedly, check power supply (needs 5V stable)
- Camera errors may require different frame size settings
