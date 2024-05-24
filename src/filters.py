import cv2
import numpy as np

# Function to apply Negative effect
def applyNegative(image):
    result = cv2.bitwise_not(image)
    if result.shape[2] == 1:
        result = cv2.cvtColor(result, cv2.COLOR_GRAY2BGR)
    return result

# Function to apply Sepia effect
def applySepia(image):
    sepia_matrix = np.array([[0.272, 0.534, 0.131],
                             [0.349, 0.686, 0.168],
                             [0.393, 0.769, 0.189]])

    sepia_image = cv2.transform(image, sepia_matrix.T)
    sepia_image = np.clip(sepia_image, 0, 255).astype(np.uint8)

    return sepia_image

# Function to apply Black and White effect
def applyBlackAndWhite(image):
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    result = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)
    return result

# Function to apply Gaussian Blur
def applyBlur(image):
    result = cv2.GaussianBlur(image, (15, 15), 10)
    return result

# Function to apply Cartoon effect
def applyCartoonEffect(image):
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    edges = cv2.medianBlur(gray, 7)
    edges = cv2.adaptiveThreshold(edges, 255, cv2.ADAPTIVE_THRESH_MEAN_C, cv2.THRESH_BINARY, 9, 2)
    edges = cv2.cvtColor(edges, cv2.COLOR_GRAY2BGR)
    result = cv2.bitwise_and(image, edges)
    return result

# Function to apply Pencil Sketch effect
def applyPencilSketch(image):
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    blur = cv2.GaussianBlur(gray, (21, 21), 0)
    sketch = cv2.divide(gray, blur, scale=256.0)
    sketch = cv2.cvtColor(sketch, cv2.COLOR_GRAY2BGR)
    return sketch

# Function to apply Thermal Vision effect
def applyThermalVision(image):
    result = cv2.applyColorMap(image, cv2.COLORMAP_JET)
    return result

# Function to apply Edge Detection effect
def applyEdgeDetection(image):
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    edges = cv2.Canny(gray, 100, 200)
    edges = cv2.cvtColor(edges, cv2.COLOR_GRAY2BGR)
    return edges
