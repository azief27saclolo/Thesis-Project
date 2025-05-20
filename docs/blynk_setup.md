# Blynk Mobile App Setup Guide

## 1. Initial Setup

1. Download Blynk app from [App Store](https://apps.apple.com/us/app/blynk-iot/id1559317868) or [Google Play](https://play.google.com/store/apps/details?id=cloud.blynk)
2. Create a Blynk account
3. Create a new project:
   - Choose ESP32 as device
   - Choose WiFi as connection type
   - Note down the Auth Token sent to your email

## 2. Dashboard Layout

Add the following widgets:

1. **Label** (connected to V0)
   - Title: "Disease Status"
   - Shows current detected disease

2. **Gauge** (connected to V1)
   - Title: "Confidence"
   - Min: 0, Max: 100
   - Shows detection confidence

3. **Value Display** (connected to V2)
   - Title: "Last Update"
   - Shows timestamp of last detection

4. **SuperChart** (connect to V0 and V1)
   - To show history of detections
   - Enable "Time" mode
   
5. **Notification Widget**
   - No pin needed - for push notifications

6. **Alarm and Sound Widgets:**
   - LED (V3): "Disease Alarm" - lights up when disease detected
   - Button (V8): "Sound Alert" - can be used with Eventor to play sounds

7. **Disease Count Displays:**
   - Value Display (V4): "Healthy Count"
   - Value Display (V5): "Early Blight Count"
   - Value Display (V6): "Late Blight Count"
   - Value Display (V7): "TYLCV Count"

8. **Sprayer Control:**
   - Button (V9): "Manual Spray" - press to activate sprayer
   - Switch (V10): "Auto Spray" - toggle automatic spraying

## 3. Configuration

In ESP32 code, update these values:
```cpp
char auth[] = "YourBlynkAuthToken";
char ssid[] = "YourWiFiName";
char password[] = "YourWiFiPassword";
```

## 4. How It Works

1. ESP32-CAM detects diseases and connects to Blynk cloud
2. Results appear immediately on your mobile app
3. Push notifications sent when diseases are detected
4. Offline detections stored on SD card
5. When connection is restored, offline data is synced

## 5. Benefits

- No need for PC or Raspberry Pi server
- Real-time mobile monitoring
- Works anywhere with internet access
- Push notifications
- Historical data visualization
- Low-cost solution
