# Building ML Models for Tomato Disease Detection

## Quick Start Guide

1. **Setup Environment**:
   ```bash
   cd d:\xamp\htdocs\Thesis proj
   python -m venv .venv
   .venv\Scripts\activate
   pip install -r model/requirements.txt
   ```

2. **Prepare Dataset Structure**:
   Place 300 images per disease category in:
   ```
   raw_dataset/
   ├── healthy_leaf/
   ├── early_blight_leaf/
   ├── late_blight_leaf/
   └── tomato_yellow_leaf_curl_virus/
   ```

3. **Process Raw Images**:
   ```bash
   python model/preprocess.py
   python model/augment_dataset.py
   python model/prepare_dataset.py
   ```

4. **Train the Model**:
   ```bash
   python model/tomato_cnn.py
   ```
   This runs a complete pipeline:
   - Creates MobileNetV2 (α=0.25) with custom classification head
   - Trains for 20 epochs with Adam optimizer
   - Fine-tunes with reduced learning rate
   - Applies INT8 quantization
   - Saves optimized TFLite model

5. **Convert for ESP32**:
   ```bash
   xxd -i esp32/model/tomato_model.tflite > esp32/tomato_model_tflite.cpp
   ```

## Key Implementation Details

- **Model**: MobileNetV2 with α=0.25 and 64×64×3 input size
- **Quantization**: INT8 reduces model from ~1.8MB to ~300KB
- **Loss Function**: Sparse categorical cross-entropy for integer class labels
- **Training Architecture**: Two-phase approach with initial training and refinement
- **Preprocessing**: CLAHE enhancement improves disease feature detection
- **Activation Function**: ReLU (Rectified Linear Unit)

### Understanding ReLU Activation

ReLU (Rectified Linear Unit) is the activation function used in our model's dense layer. It's defined mathematically as:

f(x) = max(0, x)

This simple function has several important properties that make it effective for deep learning:

1. **Computational Efficiency**: ReLU is extremely simple to compute compared to sigmoid or tanh functions, only requiring a max operation.

2. **Sparsity**: ReLU produces true zeros (not just near-zero values) for negative inputs, creating sparse activations that improve computational efficiency.

3. **Non-linearity**: Despite its simplicity, ReLU introduces the necessary non-linearity that allows neural networks to learn complex patterns.

4. **Reduced Vanishing Gradient Problem**: Unlike sigmoid activations, ReLU doesn't saturate in the positive region, which helps prevent the vanishing gradient problem during backpropagation.

In our tomato disease classifier, ReLU is specifically used in the dense layer with 16 neurons, allowing the network to learn non-linear representations from the convolutional features extracted by MobileNetV2, while maintaining efficient computation necessary for the ESP32-CAM's limited resources.

## Building the ML Model - Overview

The tomato disease detection model is built through a streamlined process starting with dataset preparation, where 300 images per disease class are collected in the raw_dataset folder and organized into distinct disease categories (healthy_leaf, early_blight_leaf, late_blight_leaf, and tomato_yellow_leaf_curl_virus). The preprocessing pipeline enhances these images using CLAHE for better feature extraction and resizes them to 64×64 pixels for computational efficiency. Data augmentation then generates three variants per original image to improve model robustness. After preparing the dataset structure, the model is constructed using MobileNetV2 architecture with a reduced alpha value of 0.25, customized with a lightweight classification head consisting of GlobalAveragePooling2D, a 16-neuron dense layer with ReLU activation, and dropout for regularization. Training follows a two-phase approach: initial training for 20 epochs with Adam optimizer (learning rate 0.001) and sparse categorical cross-entropy loss, followed by refinement at a lower learning rate (1e-4) with early stopping to prevent overfitting. The trained model undergoes INT8 quantization, reducing its size from approximately 1.8MB to about 300KB while maintaining accuracy between 87-90%. Finally, the optimized TFLite model is converted to a C++ array using xxd for direct integration into the ESP32-CAM firmware, creating a balanced solution between detection accuracy and the severe resource constraints of edge deployment.
