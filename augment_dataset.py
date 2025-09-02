import cv2
import numpy as np
import os
import tensorflow as tf
import argparse

def create_augmentation_layer():
    """Create augmentation using tf.keras.Sequential but with more color preservation"""
    return tf.keras.Sequential([
        tf.keras.layers.RandomRotation(0.15),        # Reduced rotation (was 0.2)
        tf.keras.layers.RandomTranslation(0.1, 0.1), # Reduced translation (was 0.2)
        tf.keras.layers.RandomZoom(0.1),            # Reduced zoom (was 0.2)
        tf.keras.layers.RandomFlip("horizontal"),   # Keep horizontal flip
        # Removed RandomBrightness which can cause color issues
    ])

def augment_dataset(input_dir, output_dir, samples_per_image=5):
    """Augment images in the dataset with better color preservation"""
    augmentation_layer = create_augmentation_layer()
    
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    for class_name in os.listdir(input_dir):
        class_dir = os.path.join(input_dir, class_name)
        if not os.path.isdir(class_dir):
            continue
            
        output_class_dir = os.path.join(output_dir, class_name)
        os.makedirs(output_class_dir, exist_ok=True)
        
        print(f"\nProcessing {class_name}...")
        
        images = [f for f in os.listdir(class_dir) 
                 if f.lower().endswith(('.png', '.jpg', '.jpeg'))]
        
        for img_name in images:
            # Load image directly with OpenCV to preserve color
            img_path = os.path.join(class_dir, img_name)
            orig_cv2 = cv2.imread(img_path)
            
            if orig_cv2 is None:
                print(f"Warning: Could not read {img_path}")
                continue
                
            # Convert BGR to RGB for TensorFlow processing
            orig_rgb = cv2.cvtColor(orig_cv2, cv2.COLOR_BGR2RGB)
            
            # Resize to 96x96 to match model input size
            orig_rgb = cv2.resize(orig_rgb, (96, 96))
            
            # Create TensorFlow tensor
            img = tf.convert_to_tensor(orig_rgb, dtype=tf.float32) / 255.0
            img = tf.expand_dims(img, 0)
            
            # Save original image (resized to 96x96)
            base_name = os.path.splitext(img_name)[0]
            original_path = os.path.join(output_class_dir, f"{base_name}_original.jpg")
            cv2.imwrite(original_path, cv2.cvtColor(orig_rgb, cv2.COLOR_RGB2BGR))  # Save resized image
            
            # Generate augmented images with better color preservation
            successful_augmentations = 0
            max_attempts = samples_per_image * 3  # Allow retries for bad images
            attempt = 0
            
            while successful_augmentations < samples_per_image and attempt < max_attempts:
                attempt += 1
                aug_img = augmentation_layer(img, training=True)
                
                # Convert back to uint8 and BGR for OpenCV
                aug_img_np = (aug_img[0].numpy() * 255).astype(np.uint8)
                
                # Verify this is a good augmentation by checking color range and contrast
                std_dev = np.std(aug_img_np)
                
                # Skip very dark, bright, or low contrast images
                if std_dev < 25:  # Increased threshold for better contrast
                    continue
                    
                # Check if any color channel is too dominant (causing color tints)
                rgb_means = np.mean(aug_img_np, axis=(0,1))
                max_color_ratio = max(rgb_means) / (np.mean(rgb_means) + 1e-5)
                if max_color_ratio > 1.5:  # If one color is too dominant
                    print(f"Skipping image with color imbalance for {img_name}")
                    continue
                    
                aug_img_bgr = cv2.cvtColor(aug_img_np, cv2.COLOR_RGB2BGR)
                output_path = os.path.join(output_class_dir, f"{base_name}_aug_{successful_augmentations+1}.jpg")
                cv2.imwrite(output_path, aug_img_bgr)
                successful_augmentations += 1
                
            print(f"Generated {successful_augmentations} good augmented images for {img_name}")

if __name__ == "__main__":
    # Set up argument parser
    parser = argparse.ArgumentParser(description='Augment tomato disease dataset with configurable options')
    parser.add_argument('--augment', action='store_true', help='Enable dataset augmentation')
    parser.add_argument('--samples', type=int, default=3, help='Number of augmented samples to generate per original image')
    parser.add_argument('--input_dir', type=str, help='Custom input directory path (optional)')
    parser.add_argument('--output_dir', type=str, help='Custom output directory path (optional)')
    args = parser.parse_args()
    
    # Use default or custom paths
    PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
    INPUT_DIR = args.input_dir if args.input_dir else os.path.join(PROJECT_ROOT, "raw_dataset")
    OUTPUT_DIR = args.output_dir if args.output_dir else os.path.join(PROJECT_ROOT, "augmented_dataset")
    SAMPLES_PER_IMAGE = args.samples
    
    print(f"Input directory: {INPUT_DIR}")
    print(f"Output directory: {OUTPUT_DIR}")
    
    # Check if augmentation is enabled
    if args.augment:
        print(f"Augmentation enabled. Generating {SAMPLES_PER_IMAGE} augmented images per original image")
        augment_dataset(INPUT_DIR, OUTPUT_DIR, SAMPLES_PER_IMAGE)
        print("\nAugmentation completed!")
    else:
        print("Augmentation skipped. Use --augment to enable augmentation.")
        print("\nTo generate augmented images, run:")
        print("python augment_dataset.py --augment")
        print("\nAdditional options:")
        print("  --samples N    : Generate N samples per original image")
        print("  --input_dir DIR: Use custom input directory")
        print("  --output_dir DIR: Use custom output directory")
        print("\nExample:")
        print("python augment_dataset.py --augment --samples 5 --input_dir custom_raw_data --output_dir custom_output")
