import socket
import struct
import cv2
import numpy as np
import tkinter as tk
from tkinter import filedialog

SERVER_ADDRESS = ('localhost', 8080)

def send_image(client_socket, image):
    encoded_image = cv2.imencode('.jpg', image)[1].tobytes()
    client_socket.sendall(struct.pack('l', len(encoded_image)))
    client_socket.sendall(encoded_image)

def receive_image(client_socket, width, height):
    total_size = width * height
    received_size = 0
    data = b''

    # Receive image data in chunks until all data is received
    while received_size < total_size:
        chunk = client_socket.recv(min(4096, total_size - received_size))
        if not chunk:
            raise ConnectionError("Connection closed by server")
        data += chunk
        received_size += len(chunk)

    if received_size != total_size:
        raise ValueError("Received incomplete image data")

    # Convert received data to grayscale image
    grayscale_image = np.frombuffer(data, dtype=np.uint8).reshape((height, width))
    return grayscale_image

def main():
    root = tk.Tk()
    root.withdraw()  # Hide the main window

    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        client_socket.connect(SERVER_ADDRESS)
        print("Connected to server.")

        # User authentication
        username = input("Enter username: ")
        password = input("Enter password: ")
        credentials = f"{username},{password}"
        client_socket.sendall(credentials.encode())
        
        # Receive login result
        login_result = client_socket.recv(1024).decode()
        if login_result == "FAIL":
            print("Login failed. Exiting.")
            return
        else:
            print("Login successful. Proceeding.")

        file_path = filedialog.askopenfilename(title="Select an image", filetypes=[("Image files", "*.jpg *.jpeg *.png *.bmp *.gif")])
        if not file_path:
            print("No image selected. Exiting.")
            return

        image = cv2.imread(file_path)
        cv2.imshow("Selected Image", image)
        cv2.waitKey(0)

        height, width, channels = image.shape

        send_image(client_socket, image)
        print("Image sent successfully.")
        grayscale_image = receive_image(client_socket, width, height)
        print("Image received successfully.")

        # Display grayscale image
        cv2.imshow("Grayscale Image", grayscale_image)
        cv2.waitKey(0)

    except Exception as e:
        print(f"Error: {e}")
    finally:
        client_socket.close()

if __name__ == "__main__":
    main()
