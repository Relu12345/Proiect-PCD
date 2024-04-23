import socket
import cv2
import numpy as np
import tkinter as tk
from tkinter import filedialog

SERVER_ADDRESS = ('localhost', 8080)

def send_image(client_socket, image):
    encoded_image = cv2.imencode('.jpg', image)[1].tostring()
    client_socket.sendall(encoded_image)

def receive_image(client_socket):
    image_bytes = b''
    while True:
        chunk = client_socket.recv(4096)
        if not chunk:
            break
        image_bytes += chunk
    return image_bytes

def main():
    root = tk.Tk()
    root.withdraw()  # Hide the main window

    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        client_socket.connect(SERVER_ADDRESS)
        print("Connected to server.")

        file_path = filedialog.askopenfilename(title="Select an image", filetypes=[("Image files", "*.jpg *.jpeg *.png *.bmp *.gif")])
        if not file_path:
            print("No image selected. Exiting.")
            return

        image = cv2.imread(file_path)
        cv2.imshow("Selected Image", image)
        cv2.waitKey(0)

        send_image(client_socket, image)
        print("Image sent successfully.")

        received_bytes = receive_image(client_socket)
        received_image = cv2.imdecode(np.frombuffer(received_bytes, np.uint8), cv2.IMREAD_GRAYSCALE)
        cv2.imshow("Converted Image", received_image)
        cv2.waitKey(0)

    except Exception as e:
        print(f"Error: {e}")
    finally:
        client_socket.close()

if __name__ == "__main__":
    main()
