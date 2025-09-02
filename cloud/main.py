import os
import json
import tensorflow as tf
import numpy as np
from flask import jsonify
from PIL import Image
from io import BytesIO
import base64
import functions_framework

# Path to the saved model directory relative to the function's root
MODEL_DIR = 'model'
CLASS_INFO_PATH = os.path.join(MODEL_DIR, 'class_info.json')

# Load model on cold start
model = None
class_names = None

def load_model():
    global model, class_names
    
    # Load the model
    model = tf.saved_model.load(MODEL_DIR)
    model_func = model.signatures["serve"]
    
    # Load class names
    with open(CLASS_INFO_PATH, 'r') as f:
        class_info = json.load(f)
        class_names = class_info["classes"]
    
    print(f"Model loaded successfully. Class names: {class_names}")
    return model_func

# Preprocess image to match model's expected input
def preprocess_image(image_data):
    # Decode base64 image
    img = Image.open(BytesIO(base64.b64decode(image_data)))
    
    # Resize and convert to array
    img = img.resize((96, 96))
    img_array = tf.image.convert_image_dtype(np.array(img), tf.float32)
    
    # Add batch dimension
    img_array = tf.expand_dims(img_array, 0)
    
    return img_array

@functions_framework.http
def detect_tomato_disease(request):
    global model, class_names
    
    # Load model if not already loaded (handles cold starts)
    if model is None:
        load_model()
    
    # Set CORS headers for the preflight request
    if request.method == 'OPTIONS':
        # Allows GET requests from any origin with the Content-Type
        # header and caches preflight response for an 3600s
        headers = {
            'Access-Control-Allow-Origin': '*',
            'Access-Control-Allow-Methods': 'POST',
            'Access-Control-Allow-Headers': 'Content-Type',
            'Access-Control-Max-Age': '3600'
        }
        return ('', 204, headers)
    
    # Set CORS headers for the main request
    headers = {'Access-Control-Allow-Origin': '*'}
    
    # Check if request is properly formed
    request_json = request.get_json(silent=True)
    if not request_json or 'image' not in request_json:
        return jsonify({
            'error': 'Invalid request. Please provide an image in base64 format.'
        }), 400, headers
    
    # Get the base64 encoded image
    image_data = request_json['image']
    if not image_data:
        return jsonify({
            'error': 'Empty image data'
        }), 400, headers
    
    try:
        # Preprocess the image
        img_tensor = preprocess_image(image_data)
        
        # Run inference
        predictions = model(img_tensor)
        
        # Get the results
        if isinstance(predictions, dict):
            # For saved_model format
            prediction_values = predictions[next(iter(predictions))].numpy()[0]
        else:
            # For direct model format
            prediction_values = predictions.numpy()[0]
        
        # Get the predicted class
        predicted_class_idx = np.argmax(prediction_values)
        predicted_class = class_names[predicted_class_idx]
        confidence = float(prediction_values[predicted_class_idx])
        
        # Create result dictionary
        results = {
            'class': predicted_class,
            'confidence': confidence,
            'all_probabilities': {
                class_name: float(prediction_values[i]) 
                for i, class_name in enumerate(class_names)
            }
        }
        
        return jsonify(results), 200, headers
        
    except Exception as e:
        return jsonify({
            'error': f'Error processing image: {str(e)}'
        }), 500, headers
