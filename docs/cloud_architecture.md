# Project Architecture: Cloud-Based Approach

This document outlines the architectural changes made to the tomato disease detection project, shifting from an edge computing approach to a cloud-based solution.

## Previous Architecture (Edge Computing)

The previous architecture used the ESP32-CAM for both image capture and inference:

1. ESP32-CAM captures images of tomato plants
2. TensorFlow Lite model runs directly on the ESP32-CAM
3. Results are sent to Blynk app for viewing
4. Power consumption is higher due to on-device inference

## New Architecture (Cloud-Based)

The new architecture offloads the processing to Google Cloud:

1. ESP32-CAM captures images of tomato plants
2. Images are uploaded to Firebase Storage
3. Google Cloud Functions process images using TensorFlow
4. Results are stored in Firebase Realtime Database
5. Blynk app retrieves and displays results

## Components Overview

### 1. ESP32-CAM
- **Primary Role**: Image capture device
- **Secondary Functions**:
  - Connects to WiFi
  - Uploads images to Firebase Storage
  - Receives commands from Firebase and Blynk
  - Reports status to Firebase
  - Manages power (sleep/wake cycles)

### 2. Firebase
- **Storage**: Stores captured images
- **Realtime Database**: Stores detection results and device status
- **Authentication**: Secures access to data
- **Cloud Messaging**: Sends notifications for disease detections

### 3. Google Cloud Functions
- **Image Analysis**: Processes images using TensorFlow model
- **Database Updates**: Writes results to Firebase
- **Notification Triggers**: Sends alerts when diseases are detected
- **API Endpoints**: Provides data for the Blynk app

### 4. Blynk IoT Platform
- **Mobile App**: User interface for monitoring and control
- **Widgets**: Display detection results and device status
- **Controls**: Adjust capture frequency, trigger manual captures
- **Notifications**: Alert user to disease detections

## Data Flow

1. **Image Capture**:
   ```
   ESP32-CAM → Firebase Storage
   ```

2. **Image Analysis**:
   ```
   Firebase Storage → Google Cloud Functions → Firebase Database
   ```

3. **Results Display**:
   ```
   Firebase Database → Blynk App
   ```

4. **Commands/Control**:
   ```
   Blynk App → Firebase Database → ESP32-CAM
   ```

## Advantages of Cloud Architecture

1. **Reduced Power Consumption**:
   - ESP32-CAM uses significantly less power without running inference
   - Longer battery life for field deployments
   - Potential for solar-powered operation

2. **Improved Accuracy**:
   - Full-sized model runs in the cloud without memory constraints
   - Higher resolution images can be processed
   - Model updates can be deployed without updating devices

3. **Scalability**:
   - System can handle multiple ESP32-CAM devices
   - Easy to add more processing power as needed
   - Centralized data collection across multiple fields/locations

4. **Enhanced Features**:
   - Historical data analysis
   - Advanced visualizations
   - Integration with other agricultural systems
   - Multi-user access to data

5. **Maintenance and Updates**:
   - Model updates happen in the cloud, not on devices
   - Bug fixes can be deployed without updating firmware
   - Data backups and redundancy

## Implementation Timeline

1. **Phase 1**: ESP32-CAM code for Firebase integration
   - Image capture and upload functionality
   - Power management implementation
   - Basic Blynk integration

2. **Phase 2**: Cloud Functions development
   - Model deployment in Google Cloud
   - Image processing pipeline
   - Results storage in Firebase

3. **Phase 3**: Blynk app configuration
   - Dashboard setup for cloud data
   - Controls for ESP32-CAM
   - Notification system

4. **Phase 4**: Testing and optimization
   - End-to-end system testing
   - Power consumption optimization
   - Performance tuning

## Future Extensions

1. **Multi-Camera Support**:
   - Deploy multiple ESP32-CAMs across different fields
   - Centralized monitoring of all devices

2. **Advanced Analytics**:
   - Disease progression tracking over time
   - Correlation with weather data
   - Predictive models for disease outbreaks

3. **Automated Interventions**:
   - Integration with automated sprayers or irrigation systems
   - Scheduled treatments based on detection results

4. **Web Dashboard**:
   - Browser-based monitoring interface
   - Advanced data visualization
   - Administrative features
