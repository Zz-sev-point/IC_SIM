import tensorflow as tf
print("Num GPUs Available: ", len(tf.config.experimental.list_physical_devices('GPU')))

from tensorflow import keras
import numpy as np
import matplotlib.pyplot as plt


(X_train,y_train),(X_test,y_test) = keras.datasets.mnist.load_data()

x_train, x_test = X_train / 255.0, X_test / 255.0

# Define the model
model = keras.Sequential([
    keras.layers.Flatten(input_shape=(28, 28)),  # Flatten 28x28 images to 1D
    keras.layers.Dense(512, activation='relu'),  # Hidden layer with 512 neurons
    keras.layers.Dense(32, activation='relu'),  # Hidden layer with 32 neurons
    keras.layers.Dense(10, activation='softmax')  # Output layer (10 classes)
])

model.summary()

# Compile the model
model.compile(optimizer='adam',
              loss='sparse_categorical_crossentropy',
              metrics=['accuracy'])

model.fit(x_train, y_train, epochs=5)

test_loss, test_acc = model.evaluate(x_test, y_test)
print(f"Test Accuracy: {test_acc:.4f}")

predictions = model.predict(x_test)

# Example: Predict the first test image
predicted_label = np.argmax(predictions[0])

# Display the image and the predicted label
plt.imshow(x_test[0], cmap=plt.cm.binary)
plt.title(f"Predicted Label: {predicted_label}")
plt.savefig("mnist_prediction.png")
