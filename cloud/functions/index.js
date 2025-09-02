const functions = require("firebase-functions");
const admin = require("firebase-admin");
const tf = require("@tensorflow/tfjs-node");
const { Storage } = require("@google-cloud/storage");
const os = require("os");
const path = require("path");
const fs = require("fs");
const sharp = require("sharp");

// Initialize Firebase
admin.initializeApp();
const storage = new Storage();
const db = admin.firestore();

// Disease class names (make sure these match your model's output classes)
const CLASS_NAMES = [
  "healthy_leaf",
  "early_blight_leaf",
  "late_blight_leaf",
  "tomato_yellow_leaf_curl_virus"
];

// Global variable to store the model
let model;

// Load model (happens once when Cloud Function first executes)
async function loadModel() {
  if (!model) {
    console.log("Loading TensorFlow model...");
    // Path to your model in the Cloud Function's deployment
    model = await tf.node.loadSavedModel("./model");
    console.log("Model loaded successfully");
  }
  return model;
}

// Preprocess image
async function preprocessImage(imagePath) {
  // Resize and normalize image (same as in your Python preprocessing)
  try {
    // Resize to 96x96 (match your model's input size)
    const imageBuffer = await sharp(imagePath)
      .resize(96, 96)
      .toBuffer();
    
    // Convert to tensor and normalize
    const tensor = tf.node.decodeImage(imageBuffer, 3);
    // Expand dimensions to create batch of 1 and normalize to 0-1
    const normalized = tf.expandDims(tensor, 0).div(255.0);
    
    return normalized;
  } catch (error) {
    console.error("Error preprocessing image:", error);
    throw error;
  }
}

// Classify image
async function classifyImage(tensor) {
  const model = await loadModel();
  
  // Run inference
  const predictions = await model.predict(tensor);
  
  // Get the prediction with highest probability
  const predictionData = await predictions.data();
  const idx = predictionData.indexOf(Math.max(...predictionData));
  
  return {
    class: CLASS_NAMES[idx],
    confidence: predictionData[idx],
    allProbabilities: Array.from(predictionData).map((p, i) => ({
      class: CLASS_NAMES[i],
      probability: p
    }))
  };
}

// Cloud Function triggered when a new image is added to the database
exports.analyzeTomatoImage = functions.firestore
  .document("images/{imageId}")
  .onCreate(async (snapshot, context) => {
    const imageData = snapshot.data();
    
    // Only process images with status "pending_analysis"
    if (imageData.status !== "pending_analysis") {
      console.log("Image already processed or not ready for analysis");
      return null;
    }
    
    console.log(`Processing image: ${imageData.image_path}`);
    
    try {
      // Update status to "processing"
      await snapshot.ref.update({ status: "processing" });
      
      // Download image from Firebase Storage
      const tempLocalFile = path.join(os.tmpdir(), imageData.image_path);
      const bucket = storage.bucket("gs://" + imageData.image_path.split("/")[0]);
      const fileName = imageData.image_path.split("/")[1];
      
      await bucket.file(fileName).download({ destination: tempLocalFile });
      console.log("Image downloaded to:", tempLocalFile);
      
      // Preprocess image
      const inputTensor = await preprocessImage(tempLocalFile);
      
      // Classify image
      const result = await classifyImage(inputTensor);
      
      // Update database with results
      await snapshot.ref.update({
        status: "completed",
        classification: result.class,
        confidence: result.confidence,
        allProbabilities: result.allProbabilities,
        processed_at: admin.firestore.FieldValue.serverTimestamp()
      });
      
      // Update device's latest result
      await db.collection("devices").doc(imageData.device_id).update({
        latest_result: `${result.class} (${(result.confidence * 100).toFixed(2)}%)`,
        latest_analysis: admin.firestore.FieldValue.serverTimestamp()
      });
      
      // Send notification if disease detected (not healthy)
      if (result.class !== "healthy_leaf" && result.confidence > 0.7) {
        const notification = {
          title: "Tomato Disease Detected!",
          body: `${result.class} detected with ${(result.confidence * 100).toFixed(2)}% confidence`,
          token: imageData.device_id  // FCM token would be stored with device
        };
        
        // Store notification in database for retrieval by Blynk app
        await db.collection("notifications").add({
          device_id: imageData.device_id,
          title: notification.title,
          message: notification.body,
          image_ref: imageData.image_path,
          created_at: admin.firestore.FieldValue.serverTimestamp(),
          read: false
        });
      }
      
      // Clean up temporary file
      fs.unlinkSync(tempLocalFile);
      
      return null;
    } catch (error) {
      console.error("Error processing image:", error);
      
      // Update with error status
      await snapshot.ref.update({
        status: "error",
        error_message: error.message,
        processed_at: admin.firestore.FieldValue.serverTimestamp()
      });
      
      return null;
    }
  });

// HTTP endpoint for testing the model directly
exports.testModel = functions.https.onRequest(async (req, res) => {
  try {
    if (req.method !== "POST") {
      res.status(405).send("Method not allowed");
      return;
    }
    
    // Check if image was uploaded
    if (!req.files || !req.files.image) {
      res.status(400).send("No image uploaded");
      return;
    }
    
    const tempLocalFile = path.join(os.tmpdir(), "test_image.jpg");
    fs.writeFileSync(tempLocalFile, req.files.image.data);
    
    // Process image
    const inputTensor = await preprocessImage(tempLocalFile);
    const result = await classifyImage(inputTensor);
    
    // Clean up
    fs.unlinkSync(tempLocalFile);
    
    // Return results
    res.status(200).send({
      result: result.class,
      confidence: result.confidence,
      allProbabilities: result.allProbabilities
    });
  } catch (error) {
    console.error("Error in test endpoint:", error);
    res.status(500).send("Error processing image: " + error.message);
  }
});

// Export model to TFLite
exports.exportModelToTFLite = functions.https.onRequest(async (req, res) => {
  if (req.method !== "POST") {
    res.status(405).send("Method not allowed");
    return;
  }
  
  try {
    const model = await loadModel();
    
    // TensorFlow.js doesn't directly support TFLite conversion
    // This would require additional libraries or services
    
    res.status(200).send({
      message: "For TFLite conversion, use the Python script provided in the repository",
      modelLoaded: true
    });
  } catch (error) {
    console.error("Error loading model:", error);
    res.status(500).send("Error loading model: " + error.message);
  }
});

// Get disease statistics
exports.getDiseaseStats = functions.https.onRequest(async (req, res) => {
  try {
    // Query for completed analyses in the last 30 days
    const thirtyDaysAgo = admin.firestore.Timestamp.fromDate(
      new Date(Date.now() - 30 * 24 * 60 * 60 * 1000)
    );
    
    const querySnapshot = await db.collection("images")
      .where("status", "==", "completed")
      .where("processed_at", ">=", thirtyDaysAgo)
      .get();
    
    // Count occurrences of each disease
    const stats = {};
    CLASS_NAMES.forEach(className => {
      stats[className] = 0;
    });
    
    querySnapshot.forEach(doc => {
      const data = doc.data();
      if (data.classification) {
        stats[data.classification] = (stats[data.classification] || 0) + 1;
      }
    });
    
    // Calculate percentages
    const total = Object.values(stats).reduce((sum, count) => sum + count, 0);
    const percentages = {};
    
    Object.keys(stats).forEach(className => {
      percentages[className] = total > 0 ? (stats[className] / total * 100).toFixed(2) + "%" : "0%";
    });
    
    res.status(200).send({
      counts: stats,
      percentages: percentages,
      total: total,
      period: "Last 30 days"
    });
  } catch (error) {
    console.error("Error getting disease stats:", error);
    res.status(500).send("Error getting statistics: " + error.message);
  }
});
