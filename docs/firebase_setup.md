# Firebase Setup Guide for Tomato Disease Detection

This guide walks you through setting up Firebase for your cloud-based tomato disease detection system.

## 1. Create a Firebase Project

1. Go to the [Firebase Console](https://console.firebase.google.com/)
2. Click "Add project"
3. Enter a project name (e.g., "Tomato Disease Detection")
4. Choose whether to enable Google Analytics (recommended)
5. Follow the prompts to complete project creation

## 2. Set Up Firebase Authentication

1. In the Firebase Console, navigate to "Authentication"
2. Click "Get started"
3. Enable the "Email/Password" sign-in method
4. Add a test user for development:
   - Click "Add user"
   - Enter an email and password

## 3. Create Firebase Realtime Database

1. In the Firebase Console, navigate to "Realtime Database"
2. Click "Create Database"
3. Start in test mode for development (you'll update security rules later)
4. Choose a database location close to your target users

## 4. Set Up Firebase Storage

1. In the Firebase Console, navigate to "Storage"
2. Click "Get started"
3. Start in test mode for development
4. Choose a storage location close to your target users

## 5. Implement Security Rules

Once your development is complete, update your security rules:

### Realtime Database Rules

```json
{
  "rules": {
    "devices": {
      "$deviceId": {
        ".read": "auth != null",
        ".write": "auth != null || $deviceId.contains(request.auth.uid)"
      }
    },
    "images": {
      ".read": "auth != null",
      ".write": "auth != null"
    },
    "notifications": {
      ".read": "auth != null",
      ".write": "auth != null"
    }
  }
}
```

### Storage Rules

```
rules_version = '2';
service firebase.storage {
  match /b/{bucket}/o {
    match /{allPaths=**} {
      allow read: if request.auth != null;
      allow write: if request.auth != null;
    }
  }
}
```

## 6. Set Up Firebase Cloud Functions

1. Install Firebase CLI:
   ```bash
   npm install -g firebase-tools
   ```

2. Log in to Firebase:
   ```bash
   firebase login
   ```

3. Initialize Firebase in your project directory:
   ```bash
   cd your-project-directory
   firebase init
   ```
   - Select "Functions"
   - Choose your Firebase project
   - Choose JavaScript
   - Say yes to ESLint
   - Say yes to installing dependencies

4. Copy your Cloud Functions code to the `functions` directory

5. Deploy your functions:
   ```bash
   firebase deploy --only functions
   ```

## 7. Obtain Firebase Configuration for ESP32

1. In the Firebase Console, go to Project Settings
2. Under "Your apps," click the Web app icon (</>) 
3. Register your app with a nickname (e.g., "ESP32-CAM App")
4. Copy the firebaseConfig object, which will look like:

```javascript
const firebaseConfig = {
  apiKey: "your-api-key",
  authDomain: "your-project-id.firebaseapp.com",
  databaseURL: "https://your-project-id.firebaseio.com",
  projectId: "your-project-id",
  storageBucket: "your-project-id.appspot.com",
  messagingSenderId: "your-messaging-sender-id",
  appId: "your-app-id"
};
```

5. For the ESP32, you'll need:
   - `FIREBASE_HOST`: your-project-id.firebaseio.com
   - `FIREBASE_AUTH`: Generate a database secret from Project Settings > Service Accounts > Database Secrets
   - `FIREBASE_STORAGE_BUCKET`: your-project-id.appspot.com

## 8. Firebase Database Structure

Your database will have the following structure:

```
/devices
  /ESP32CAM_[device-id]
    /status: "online"
    /last_seen: timestamp
    /latest_result: "healthy_leaf (98.2%)"
    /command: ""

/images
  /[image-id]
    /device_id: "ESP32CAM_[device-id]"
    /timestamp: timestamp
    /status: "pending_analysis" | "processing" | "completed" | "error"
    /image_path: "path/to/image.jpg"
    /classification: "healthy_leaf"
    /confidence: 0.982
    /allProbabilities: [...]
    /processed_at: timestamp

/notifications
  /[notification-id]
    /device_id: "ESP32CAM_[device-id]"
    /title: "Disease Detected!"
    /message: "early_blight_leaf detected with 92.3% confidence"
    /image_ref: "path/to/image.jpg"
    /created_at: timestamp
    /read: false
```

## 9. Testing Your Firebase Setup

1. Upload a test image to Firebase Storage
2. Manually trigger your Cloud Function
3. Check the Realtime Database for results
4. Verify that your ESP32 can connect to Firebase

## 10. Firebase Maintenance

- Monitor your [Firebase usage](https://console.firebase.google.com/project/_/usage)
- Set up budget alerts to avoid unexpected charges
- Periodically clean up old images to save storage space
- Update security rules as your project evolves
