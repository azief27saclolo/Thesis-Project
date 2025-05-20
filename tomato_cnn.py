import tensorflow as tf
import os

def create_model(num_classes):
    # Using pre-trained weights from ImageNet
    base_model = tf.keras.applications.MobileNetV2(
        weights='imagenet',        
        include_top=False,         
        input_shape=(96, 96, 3),   
        alpha=0.35                
    )
    
    # Base model layers are FROZEN initially (this is the freezing part!)
    base_model.trainable = False  # <-- THIS LINE freezes the base model
    
    model = tf.keras.Sequential([
        base_model,
        tf.keras.layers.GlobalAveragePooling2D(),
        tf.keras.layers.Dense(16, activation='relu'),
        tf.keras.layers.Dropout(0.3),
        tf.keras.layers.Dense(num_classes, activation='softmax')
    ])
    
    return model

def convert_to_tflite(model, filename, data_dir):
    # More aggressive optimization for ESP32
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.uint8
    converter.inference_output_type = tf.uint8
    
    # Add post-training quantization
    converter.optimizations = [tf.lite.Optimize.OPTIMIZE_FOR_SIZE]
    
    # Use more aggressive pruning
    converter.experimental_new_converter = True
    
    # Create a wrapper function that captures data_dir
    def representative_dataset_gen():
        train_dir = os.path.join(data_dir, 'train')
        print(f"Reading calibration data from: {train_dir}")
        
        for class_name in os.listdir(train_dir):
            class_path = os.path.join(train_dir, class_name)
            print(f"Processing calibration images from class: {class_name}")
            
            for img_name in os.listdir(class_path)[:20]:  # Limit to 20 images per class for speed
                img_path = os.path.join(class_path, img_name)
                img = tf.keras.preprocessing.image.load_img(
                    img_path, target_size=(96, 96)
                )
                x = tf.keras.preprocessing.image.img_to_array(img)
                x = x / 255.0
                x = tf.expand_dims(x, axis=0)
                yield [x]
    
    converter.representative_dataset = representative_dataset_gen
    
    # Ensure the output directory exists
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    print(f"Converting model to TFLite format with aggressive optimization...")
    
    tflite_model = converter.convert()
    with open(filename, 'wb') as f:
        f.write(tflite_model)
    print(f"TFLite model saved to: {filename}")
    print(f"Model size: {len(tflite_model) / 1024:.1f} KB")

def prepare_dataset(data_dir, img_size=(96, 96), batch_size=32):
    # Use tf.keras.utils instead of keras.preprocessing.image
    train_datagen = tf.keras.utils.image_dataset_from_directory(
        os.path.join(data_dir, 'train'),
        image_size=img_size,
        batch_size=batch_size
    )
    
    # Fix the validation data loading - don't use validation_split here since
    # we're already using a separate validation directory
    valid_datagen = tf.keras.utils.image_dataset_from_directory(
        os.path.join(data_dir, 'validation'),
        image_size=img_size,
        batch_size=batch_size
    )
    
    # Store class names from training data
    class_names = train_datagen.class_names
    print(f"Training classes: {class_names}")
    valid_classes = valid_datagen.class_names
    print(f"Validation classes: {valid_classes}")
    
    # Check if classes match between train and validation
    if set(class_names) != set(valid_classes):
        raise ValueError(f"Class mismatch between training and validation sets: {class_names} vs {valid_classes}")
    
    # Normalize the data
    normalization_layer = tf.keras.layers.Rescaling(1./255)
    train_datagen = train_datagen.map(lambda x, y: (normalization_layer(x), y))
    valid_datagen = valid_datagen.map(lambda x, y: (normalization_layer(x), y))
    
    # Data augmentation
    data_augmentation = tf.keras.Sequential([
        tf.keras.layers.RandomFlip("horizontal"),
        tf.keras.layers.RandomRotation(0.2),
    ])
    
    train_datagen = train_datagen.map(
        lambda x, y: (data_augmentation(x, training=True), y)
    )
    
    # Return class names along with datasets
    return train_datagen, valid_datagen, class_names

# Two-phase training
if __name__ == "__main__":
    # Use absolute path instead of relative path
    PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    DATA_DIR = os.path.join(PROJECT_ROOT, "data")
    MODEL_PATH = os.path.join(PROJECT_ROOT, "esp32", "model", "tomato_model.tflite")
    
    # Ensure model directory exists
    os.makedirs(os.path.dirname(MODEL_PATH), exist_ok=True)
    
    print(f"Loading data from: {DATA_DIR}")
    
    # Modified to receive class_names
    train_generator, valid_generator, class_names = prepare_dataset(DATA_DIR)
    
    # Use class_names from function return value
    num_classes = len(class_names)
    print(f"Detected {num_classes} classes: {class_names}")
    
    # Create and train model
    model = create_model(num_classes)
    
    # Phase 1: Only the classification head is trainable (dense layers)
    # because base_model.trainable = False above
    model.compile(
        optimizer=tf.keras.optimizers.Adam(0.001),
        loss='sparse_categorical_crossentropy',
        metrics=['accuracy']
    )
    
    # Training with FROZEN base model
    model.fit(train_generator, epochs=10, validation_data=valid_generator)
    
    # Phase 2: Now we start the fine-tuning by unfreezing selectively
    base_model = model.layers[0]
    base_model.trainable = True  # Unfreeze entire base model
    
    # But then REfreeze all except last 20 layers
    for layer in base_model.layers[:-20]:
        layer.trainable = False  # <-- THIS LOOP freezes early layers
        
    # Lower learning rate for fine-tuning phase
    model.compile(
        optimizer=tf.keras.optimizers.Adam(1e-5),  # Much smaller learning rate
        loss='sparse_categorical_crossentropy',
        metrics=['accuracy']
    )
    
    # Now train with the last 20 layers unfrozen
    history_fine = model.fit(
        train_generator,
        epochs=10,
        validation_data=valid_generator,
        callbacks=[tf.keras.callbacks.EarlyStopping(patience=5)]
    )
    
    # Convert and save model - pass DATA_DIR to the function
    convert_to_tflite(model, MODEL_PATH, DATA_DIR)
