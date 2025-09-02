# Tomato Disease Detection Cloud Function

This document explains how to deploy and use the Google Cloud Function for tomato disease detection.

## Overview

This cloud function provides an HTTP endpoint that accepts an image of a tomato plant leaf and returns a prediction of whether the leaf is healthy or affected by a disease (Early Blight, Late Blight, or Septoria).

The function uses a TensorFlow model based on MobileNetV2 that was trained on a dataset of tomato leaf images. The model is optimized for both accuracy and performance, making it suitable for integration with mobile apps, web applications, and IoT devices like the ESP32-CAM.

## Model Information

- **Base Model**: MobileNetV2 (pretrained on ImageNet)
- **Input Shape**: 96x96x3 (RGB images)
- **Model Optimization**: Alpha (depth multiplier) of 0.35 for reduced model size
- **Classes**: 
  - `healthy_leaf`
  - `early_blight_leaf`
  - `late_blight_leaf`
  - `septoria_leaf`
- **Additional Layers**:
  - Global Average Pooling
  - Dense layer (16 units, ReLU activation)
  - Dropout (0.3 rate)
  - Output layer (softmax activation)

## Deployment Instructions

### Prerequisites

- Google Cloud Platform account with billing enabled
- Google Cloud SDK installed (optional, but recommended)
- The `cloud/` directory from this project containing:
  - `main.py` (the function handler)
  - `model/` directory (the SavedModel and class information)

### Option 1: Deploy using Google Cloud Console

1. Go to [Google Cloud Functions](https://console.cloud.google.com/functions) in the GCP Console
2. Click **Create Function**
3. Configure the function:
   - **Name**: `tomato-disease-detection` (or any preferred name)
   - **Region**: Choose a region close to your users
   - **Trigger Type**: HTTP
   - **Authentication**: Require authentication or Allow unauthenticated (depending on your security requirements)
4. Click **Next**
5. Configure the runtime:
   - **Runtime**: Python 3.10 (or newer)
   - **Entry point**: `detect_tomato_disease`
   - **Source code**: Zip upload
   - **Upload**: Zip the contents of the `cloud/` directory (not the directory itself)
6. Create a `requirements.txt` file with the following content:
   ```
   tensorflow>=2.13.0
   pillow
   numpy
   functions-framework
   ```
7. Click **Deploy**

### Option 2: Deploy using Google Cloud SDK

1. Create a `requirements.txt` file in the `cloud/` directory with the following content:
   ```
   tensorflow>=2.13.0
   pillow
   numpy
   functions-framework
   ```
2. Open a terminal and navigate to the parent directory of `cloud/`
3. Run the following command:
   ```
   gcloud functions deploy tomato-disease-detection \
     --runtime python310 \
     --trigger-http \
     --allow-unauthenticated \
     --source ./cloud \
     --entry-point detect_tomato_disease \
     --memory 1024MB \
     --timeout 60s
   ```
4. Wait for the deployment to complete

## Testing the Function

### Using cURL

```bash
curl -X POST \
  -H "Content-Type: application/json" \
  -d '{"image": "BASE64_ENCODED_IMAGE"}' \
  https://YOUR_REGION-YOUR_PROJECT_ID.cloudfunctions.net/tomato-disease-detection
```

Replace `BASE64_ENCODED_IMAGE` with a base64-encoded image of a tomato leaf, and update the URL with your function's actual URL.

### Using Python

```python
import requests
import base64
from pathlib import Path

# Load an image and convert to base64
image_path = "path/to/tomato_leaf_image.jpg"
with open(image_path, "rb") as image_file:
    encoded_image = base64.b64encode(image_file.read()).decode('utf-8')

# Make the request
url = "https://YOUR_REGION-YOUR_PROJECT_ID.cloudfunctions.net/tomato-disease-detection"
response = requests.post(url, json={"image": encoded_image})

# Print the result
print(response.json())
```

## Expected Response

The function returns a JSON response with the following structure:

```json
{
  "class": "healthy_leaf",
  "confidence": 0.95,
  "all_probabilities": {
    "early_blight_leaf": 0.02,
    "healthy_leaf": 0.95,
    "late_blight_leaf": 0.02,
    "septoria_leaf": 0.01
  }
}
```

## Function Details

### Input Requirements

- The function accepts POST requests with a JSON body
- The JSON must contain an `image` field with a base64-encoded image
- The image should ideally be of a single tomato plant leaf, centered in the frame
- Any image size is acceptable, but the image will be resized to 96x96 pixels for processing

### CORS Support

The function includes Cross-Origin Resource Sharing (CORS) headers that allow it to be called from any origin. This is useful for web applications that need to call the function directly from a browser.

### Error Handling

The function returns appropriate HTTP status codes and error messages for different failure scenarios:

- `400 Bad Request`: Missing or invalid image data
- `500 Internal Server Error`: Error during image processing or model inference

### Cold Start Optimization

The function is designed to handle "cold starts" efficiently:

- The model is loaded only once when the function instance starts
- Subsequent requests reuse the loaded model
- The model loading status is tracked using global variables

## Integration Examples

### Web Application

```javascript
async function detectDisease(imageFile) {
  // Convert file to base64
  const base64Image = await fileToBase64(imageFile);
  
  // Call the cloud function
  const response = await fetch('YOUR_FUNCTION_URL', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ image: base64Image })
  });
  
  return await response.json();
}

function fileToBase64(file) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.readAsDataURL(file);
    reader.onload = () => {
      // Remove the data:image/jpeg;base64, prefix
      const base64 = reader.result.split(',')[1];
      resolve(base64);
    };
    reader.onerror = error => reject(error);
  });
}
```

### ESP32-CAM Integration

For ESP32-CAM devices, you can capture an image, encode it to base64, and send it to the cloud function:

```cpp
// Pseudo-code for ESP32-CAM integration
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <base64.h>

void detectTomatoDisease(uint8_t* imageBuffer, size_t imageSize) {
  // Convert image to base64
  String base64Image = base64::encode(imageBuffer, imageSize);
  
  // Prepare JSON payload
  DynamicJsonDocument doc(20000);
  doc["image"] = base64Image;
  String payload;
  serializeJson(doc, payload);
  
  // Send HTTP request
  HTTPClient http;
  http.begin("YOUR_FUNCTION_URL");
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(payload);
  
  if (httpCode == 200) {
    String response = http.getString();
    // Parse and handle the response
    DynamicJsonDocument respDoc(1024);
    deserializeJson(respDoc, response);
    
    const char* diseaseClass = respDoc["class"];
    float confidence = respDoc["confidence"];
    
    // Take action based on results
    if (strcmp(diseaseClass, "healthy_leaf") == 0) {
      // Leaf is healthy
    } else {
      // Leaf has disease
    }
  }
  
  http.end();
}
```

## Troubleshooting

### Function Returns 500 Error

- Check if the model files are properly included in your deployment package
- Verify that the model directory structure matches what's expected in the code
- Check the Cloud Function logs for specific error messages

### Model Prediction Issues

- Ensure the image is clearly visible and well-lit
- The leaf should be centered in the frame
- Verify that the image is properly encoded to base64
- Try with different images to see if the issue is image-specific

### Deployment Failures

- Make sure you have sufficient permissions in your Google Cloud project
- Check that your `requirements.txt` file contains all necessary dependencies
- Ensure you have enough quota for Cloud Functions in your project

## Resources

- [Google Cloud Functions Documentation](https://cloud.google.com/functions/docs)
- [TensorFlow SavedModel Format](https://www.tensorflow.org/guide/saved_model)
- [Base64 Image Encoding](https://developer.mozilla.org/en-US/docs/Web/API/WindowOrWorkerGlobalScope/btoa)
