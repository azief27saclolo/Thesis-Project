import tensorflow as tf
import numpy as np
import os
import json
import datetime

def load_preprocessed_data(data_dir):
    """Load preprocessed numpy arrays"""
    if not os.path.exists(data_dir):
        raise FileNotFoundError(f"Directory not found: {data_dir}")
    
    class_names = sorted(os.listdir(data_dir))
    if not class_names:
        raise ValueError(f"No class folders found in {data_dir}")
    
    images = []
    labels = []
    
    for class_idx, class_name in enumerate(class_names):
        class_dir = os.path.join(data_dir, class_name)
        if not os.path.isdir(class_dir):
            continue
            
        files = [f for f in os.listdir(class_dir) if f.endswith('.npy')]
        if not files:
            print(f"Warning: No .npy files found in {class_name}/")
            continue
            
        print(f"Loading {len(files)} images from {class_name}/")
        for img_file in files:
            img_path = os.path.join(class_dir, img_file)
            img = np.load(img_path)
            images.append(img)
            labels.append(class_idx)
    
    if not images:
        raise ValueError("No images found in any class folder!")
    
    return np.array(images), np.array(labels), class_names

def create_model(num_classes):
    base_model = tf.keras.applications.MobileNetV2(  # Updated import
        weights='imagenet',
        include_top=False,
        input_shape=(96, 96, 3)
    )
    
    model = tf.keras.Sequential([
        base_model,
        tf.keras.layers.GlobalAveragePooling2D(),
        tf.keras.layers.Dense(128, activation='relu'),
        tf.keras.layers.Dropout(0.2),
        tf.keras.layers.Dense(num_classes, activation='softmax')
    ])
    
    return model

def preprocess_image(image_path):
    # Use tf.io instead of keras.preprocessing
    img = tf.io.read_file(image_path)
    img = tf.image.decode_image(img, channels=3)
    img = tf.image.resize(img, [96, 96])
    img = tf.cast(img, tf.float32) / 255.0
    return img

def convert_to_tflite(model, output_path):
    """Convert model to TFLite format"""
    # Create a converter using the SavedModel directory
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    
    # Configure the converter
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    
    # Convert the model
    tflite_model = converter.convert()
    
    # Save the model to file
    with open(output_path, 'wb') as f:
        f.write(tflite_model)
    
    print(f"TFLite model saved to: {output_path}")

def save_for_web(model, output_dir):
    """Save model in TensorFlow.js format for web deployment"""
    try:
        import tensorflowjs as tfjs
        
        os.makedirs(output_dir, exist_ok=True)
        tfjs.converters.save_keras_model(model, output_dir)
        print(f"Model saved for web deployment to: {output_dir}")
    except ImportError:
        print("tensorflowjs not installed. Skipping web model conversion.")
        print("To enable this feature, install with: pip install tensorflowjs")

def save_for_cloud(model, output_dir):
    """Save model in SavedModel format for cloud deployment"""
    try:
        os.makedirs(output_dir, exist_ok=True)
        # Export model directly to the output directory
        model.export(output_dir)
        print(f"Model saved for cloud deployment to: {output_dir}")
    except Exception as e:
        # Fallback to direct tf.saved_model.save if model.export fails
        try:
            tf.saved_model.save(model, output_dir)
            print(f"Model saved for cloud deployment to: {output_dir}")
        except Exception as inner_e:
            raise Exception(f"Failed to save model: {str(e)}, then failed fallback: {str(inner_e)}")

if __name__ == "__main__":
    try:
        # Load preprocessed data
        PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
        processed_dir = os.path.join(PROJECT_ROOT, "processed_dataset")
        print(f"Looking for processed data in: {processed_dir}")
        
        X, y, classes = load_preprocessed_data(processed_dir)
        print(f"\nLoaded {len(X)} images from {len(classes)} classes:")
        for i, cls in enumerate(classes):
            count = np.sum(y == i)
            print(f"- {cls}: {count} images")
            
        if len(X) < 10:  # Arbitrary minimum dataset size
            print("\nWarning: Very small dataset detected!")
            print("Consider adding more images (recommended: 100+ per class)")
            
        num_classes = len(classes)
        
        # Convert labels to categorical
        y = tf.keras.utils.to_categorical(y, num_classes)
        
        # Split data
        from sklearn.model_selection import train_test_split
        X_train, X_test, y_train, y_test = train_test_split(
            X, y, test_size=0.2, random_state=42
        )
        
        # Create and compile model
        model = create_model(num_classes)
        model.compile(
            optimizer='adam',
            loss='categorical_crossentropy',
            metrics=['accuracy']
        )
        
        # Train model
        history = model.fit(
            X_train, y_train,
            epochs=10,
            validation_data=(X_test, y_test),
            callbacks=[
                tf.keras.callbacks.EarlyStopping(
                    monitor='val_accuracy',
                    patience=3,
                    restore_best_weights=True
                )
            ]
        )
        
        # Create output directories
        esp32_model_dir = os.path.join(PROJECT_ROOT, "esp32", "model")
        cloud_model_dir = os.path.join(PROJECT_ROOT, "cloud", "model")
        web_model_dir = os.path.join(PROJECT_ROOT, "web", "model")
        
        os.makedirs(esp32_model_dir, exist_ok=True)
        os.makedirs(cloud_model_dir, exist_ok=True)
        os.makedirs(web_model_dir, exist_ok=True)
        
        # 1. TFLite for ESP32 (legacy support)
        try:
            tflite_path = os.path.join(esp32_model_dir, "tomato_model.tflite")
            convert_to_tflite(model, tflite_path)
            print(f"- TFLite model saved to: {tflite_path}")
        except Exception as e:
            print(f"Error saving TFLite model: {str(e)}")
        
        # 2. SavedModel format for Google Cloud Functions
        try:
            save_for_cloud(model, cloud_model_dir)
            print(f"- Cloud model saved to: {cloud_model_dir}")
        except Exception as e:
            print(f"Error saving cloud model: {str(e)}")
        
        # 3. TensorFlow.js format for web interface
        try:
            save_for_web(model, web_model_dir)
            print(f"- Web model saved to: {web_model_dir}")
        except Exception as e:
            print(f"Error saving web model: {str(e)}")
        
        # 4. Save class names for reference
        try:
            class_info = {
                "classes": classes,
                "input_shape": [96, 96, 3],
                "version": "1.0",
                "date_trained": datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            }
            
            class_info_path = os.path.join(cloud_model_dir, "class_info.json")
            with open(class_info_path, 'w') as f:
                json.dump(class_info, f, indent=2)
            print(f"- Class info saved to: {class_info_path}")
        except Exception as e:
            print(f"Error saving class info: {str(e)}")
        
        print("\nModel training completed successfully!")
        
    except Exception as e:
        print(f"\nError: {str(e)}")
        print("\nPlease ensure:")
        print("1. You have run preprocess.py first")
        print("2. You have images in your raw_dataset folder")
        print("3. The processed_dataset folder exists and contains .npy files")
        exit(1)
