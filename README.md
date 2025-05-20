# Raw Dataset Organization

Total Required Images: 2,100 images (300 per class)

/raw_dataset
├── healthy_fruit/         (300 images)  # Healthy tomato fruits
├── healthy_leaf/          (300 images)  # Healthy tomato leaves
├── early_blight_leaf/     (300 images)  # Early blight on leaves
├── early_blight_fruit/    (300 images)  # Early blight on fruits
├── late_blight_leaf/      (300 images)  # Late blight on leaves
├── late_blight_fruit/     (300 images)  # Late blight on fruits
└── tylcv/                (300 images)  # Yellow leaf curl virus

Naming Convention:
- Format: [class]_[3-digit-number].jpg
- Example: healthy_001.jpg to healthy_300.jpg

Each image should be:
- High quality and clearly showing leaf symptoms
- In common formats (jpg/png)
- Properly labeled
- Unprocessed/unaugmented
