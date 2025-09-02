# Blynk Mobile App Setup Guide for Cloud-Based System

## 1. Initial Setup

1. Download Blynk app from [App Store](https://apps.apple.com/us/app/blynk-iot/id1559317868) or [Google Play](https://play.google.com/store/apps/details?id=cloud.blynk)
2. Create a Blynk account
3. Create a new project:
   - Choose ESP32 as device
   - Choose WiFi as connection type
   - Note down the Auth Token sent to your email

## 2. Dashboard Layout for Cloud Architecture

Add the following widgets:

1. **Timer Widget** (connected to V1)
   - Title: "Capture Interval"
   - Set as a slider from 1 to 120 (minutes)
   - Controls how often the ESP32-CAM captures images

2. **Switch Widget** (connected to V2)
   - Title: "Auto Capture"
   - Enables/disables automatic image capture

3. **Button Widget** (connected to V3)
   - Title: "Capture Now"
   - Mode: Push
   - Triggers immediate image capture

4. **Value Display Widget** (connected to V4)
   - Title: "Latest Result"
   - Shows most recent disease detection result from Google Cloud

5. **Gauge Widget** (connected to V5)
   - Title: "Battery"
   - Range: 0-100%
   - Shows ESP32-CAM battery level

6. **SuperChart Widget** (connected to V6)
   - Title: "Detection History"
   - Shows history of detection results over time
   - Pulls data from Firebase via webhook

7. **Image Gallery Widget**
   - Shows recent captured images from Firebase Storage
   - Configure via webhook to your Firebase storage URL

8. **Notification Widget**
   - For receiving push notifications when diseases are detected
   - Connected to Firebase Cloud Messaging

## 3. Configuration for Cloud System

In ESP32 code, update these values:
```cpp
// Blynk credentials
#define BLYNK_AUTH "YourBlynkAuthToken"

// Firebase credentials
#define FIREBASE_HOST "your-project-id.firebaseio.com"
#define FIREBASE_AUTH "your-firebase-database-secret"
#define FIREBASE_STORAGE_BUCKET "your-project-id.appspot.com"

// WiFi credentials
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"
```

## 4. How the Cloud System Works

1. ESP32-CAM captures images and uploads to Firebase Storage
2. Google Cloud Functions analyze images using TensorFlow model
3. Detection results are stored in Firebase Realtime Database
4. Blynk app retrieves and displays results from Firebase
5. Push notifications are sent via Firebase Cloud Messaging

## 5. Benefits of the Cloud Approach

- ESP32-CAM battery lasts longer (no local processing)
- More accurate disease detection (using full-sized ML model)
- Access to advanced analytics in the cloud
- Dashboard accessible from any device anywhere
- Scalable to multiple cameras/fields
- Historical data stored indefinitely
- Integration with other cloud services possible

## 6. Webhooks for Firebase Integration

To set up Firebase to Blynk integration:

1. In the Blynk Web Console, create a new Webhook:
   - Name: "FirebaseResults"
   - URL: Your Firebase function URL that returns detection results
   - Method: GET
   - Content Type: application/json

2. Configure the Webhook to run every 5 minutes or when triggered
   - This will pull the latest data from Firebase

3. Map the Webhook response to your Blynk virtual pins:
   - V4: Latest disease result
   - V6: Historical data for chart
   - Image Gallery: URLs to detected images

## 7. Additional Features

- **Image Viewer**: Tap on detection results to view the full image
- **Disease Information**: Add buttons linked to treatment information
- **Notification Settings**: Configure alert thresholds
- **Data Export**: Export detection history to CSV
- **Multiple Device Management**: Monitor several ESP32-CAMs from one dashboard
