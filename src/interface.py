import ctypes
import struct
import sys
import tkinter as tk
from tkinter import messagebox
from tkinter import filedialog
import cv2
import numpy as np
import io
from PIL import Image, ImageTk, ImageOps
import client

CHUNK_SIZE = 1024

currentPostIndex = 0
posts = []

def send_image(client_socket, image_data):
    image_size = len(image_data)
    # Send the image size
    client_socket.sendall(struct.pack('Q', image_size))

    # Send the image data in chunks
    sent_bytes = 0
    while sent_bytes < image_size:
        chunk = image_data[sent_bytes:sent_bytes + CHUNK_SIZE]
        client_socket.sendall(chunk)
        sent_bytes += len(chunk)
    print("Image sent successfully")

def receive_image(client_socket):
    # Receive the image size
    image_size_data = receive_all(client_socket, 8)
    if not image_size_data:
        return None
    image_size = struct.unpack('Q', image_size_data)[0]
    print(f"Received image size: {image_size}")

    # Receive the image data in chunks
    image_data = b''
    while len(image_data) < image_size:
        chunk = client_socket.recv(min(CHUNK_SIZE, image_size - len(image_data)))
        if not chunk:
            return None
        image_data += chunk
    print("Image received successfully")
    
    return image_data

def receive_all(sock, n):
    data = b''
    while len(data) < n:
        packet = sock.recv(n - len(data))
        if not packet:
            return None
        data += packet
    return data

def decode_image(image_data):
    img_array = np.frombuffer(image_data, dtype=np.uint8)
    img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
    return img


def on_main_screen():
    # Add logic to open the main screen window
    print("Opening Main Screen")

def send_message(client_socket, message):
    client_socket.sendall(message.encode())

def on_login(client_socket, root):
    send_message(client_socket, "1")
    username = username_entry.get()
    password = password_entry.get()
    message = username + "," + password
    send_message(client_socket, message)

    # Receive response from the server
    response_data = client_socket.recv(7)
    response = response_data.split(b'\x00', 1)[0].decode()
    if "SUCCESS" in response:
        messagebox.showinfo("Login", "Login successful!")
        root.destroy()
    
    if "BAN" in response:
        messagebox.showerror("Login Failed", "You have been banned on this account and cannot log in")
        sys.exit(1)
    
    if "FAIL" in response:
        messagebox.showerror("Login Failed", "Login failed. Please try again.")
        sys.exit(1)

def on_register(client_socket, root):
    send_message(client_socket, "0")
    username = username_entry.get()
    password = password_entry.get()
    message = username + "," + password
    send_message(client_socket, message)

    # Receive response from the server
    response_data = client_socket.recv(7)
    response = response_data.split(b'\x00', 1)[0].decode()
    if "SUCCESS" in response:
        messagebox.showinfo("Registration", "Registration successful!")
        root.destroy()
    else:
        messagebox.showerror("Registration Failed", "Registration failed. Please try again.")

def decode_image(image_data):
    # Assuming image_data is a bytes object containing the image data
    img_array = np.frombuffer(image_data, dtype=np.uint8)
    img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
    return img

def resize_and_center_image(img, max_width, max_height):
    h, w = img.shape[:2]
    aspect_ratio = w / h
    if aspect_ratio > 1:
        new_width = max_width
        new_height = int(max_width / aspect_ratio)
    else:
        new_height = max_height
        new_width = int(max_height * aspect_ratio)
    resized_img = cv2.resize(img, (new_width, new_height))
    return resized_img

def create_login_screen(client_socket):
    global username_entry, password_entry

    # Create the main window
    root = tk.Tk()
    root.title("Login Screen")
    root.geometry("400x300")
    root.configure(bg='white')

    # Create and place the main Login label
    login_label = tk.Label(root, text="Login", font=('Comic Sans MS', 32, 'bold'), bg='white')
    login_label.place(relx=0.5, y=50, anchor='center')

    # Create and place the username label and entry
    username_label = tk.Label(root, text="Username:", font=('Comic Sans MS', 12), bg='white')
    username_label.place(x=50, y=120)
    username_entry = tk.Entry(root, font=('Comic Sans MS', 12))
    username_entry.place(x=150, y=120, width=200, height=25)

    # Create and place the password label and entry
    password_label = tk.Label(root, text="Password:", font=('Comic Sans MS', 12), bg='white')
    password_label.place(x=50, y=180)
    password_entry = tk.Entry(root, show='*', font=('Comic Sans MS', 12))
    password_entry.place(x=150, y=180, width=200, height=25)

    # Create and place the login button
    login_button = tk.Button(root, text="Login", font=('Comic Sans MS', 12), bg='red', fg='white', command=lambda: on_login(client_socket, root))
    login_button.place(x=240, y=240, width=100, height=40)

    # Create and place the register button
    register_button = tk.Button(root, text="Register", font=('Comic Sans MS', 12), bg='red', fg='white', command=lambda: on_register(client_socket, root))
    register_button.place(x=50, y=240, width=160, height=40)

    # Run the application
    root.mainloop()

def create_main_screen(user, client_posts, client_sock, option):
    global currentPostIndex, posts
    posts = client_posts

    def show_post(index):
        post = posts[index]
        try:
            img = decode_image(post.image)
            img = resize_and_center_image(img, 640, 480)
            img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
            img_pil = Image.fromarray(img)
            img_tk = ImageTk.PhotoImage(image=img_pil)

            img_label.config(image=img_tk)
            img_label.image = img_tk

            username_label.config(text=post.userName)
            description_label.config(text=post.description)
            like_count_label.config(text=f"{post.likeCount} likes")
        except Exception as e:
            print(f"Error displaying image: {e}")

    def next_post():
        global currentPostIndex
        currentPostIndex = (currentPostIndex + 1) % len(posts)
        show_post(currentPostIndex)

    def previous_post():
        global currentPostIndex
        currentPostIndex = (currentPostIndex - 1 + len(posts)) % len(posts)
        show_post(currentPostIndex)

    def first_post():
        global currentPostIndex
        currentPostIndex = 0
        show_post(currentPostIndex)

    def last_post():
        global currentPostIndex
        currentPostIndex = len(posts) - 1
        show_post(currentPostIndex)

    def refresh_posts():
        global posts, currentPostIndex
        try:
            client_sock.sendall(b'G')
            new_posts = client.receive_posts(client_sock)
            if new_posts:
                posts.clear()  # Clear the existing posts
                posts.extend(new_posts)  # Add the new posts to the existing list
                currentPostIndex = 0
                show_post(currentPostIndex)
            else:
                print("No new posts received.")
        except Exception as e:
            print(f"Error refreshing posts: {e}")

    def new_post():
        root.destroy()
        create_post_screen(user, client_sock)

    def toggle_like():
        post = posts[currentPostIndex]
        post.liked = not post.liked

        if post.liked:
            post.likeCount += 1
            # Send like signal to the server
        else:
            post.likeCount -= 1
            # Send unlike signal to the server
        send_like(post.id, user.id)

        show_post(currentPostIndex)

    def send_like(post_id, user_id):
        SIGNAL_LIKE = b"L"
        send_data = SIGNAL_LIKE + struct.pack("ii", user_id, post_id)
        client_sock.sendall(send_data)

    root = tk.Tk()
    root.title("Main Screen")
    root.geometry("1200x800")
    root.configure(bg='white')

    # Refresh Button
    refresh_button = tk.Button(root, text="Refresh", command=refresh_posts, bg='pink')
    refresh_button.place(x=0, y=0)

    # Username Label
    username_label = tk.Label(root, font=('Comic Sans MS', 14), bg='white')
    username_label.place(relx=0.5, y=10, anchor=tk.N)

    # Image Frame and Label
    img_frame = tk.Frame(root, width=640, height=480, bg='white')
    img_frame.pack_propagate(0)
    img_frame.place(relx=0.5, rely=0.5, anchor=tk.CENTER, y=-50)
    img_label = tk.Label(img_frame, bg='white')
    img_label.pack()

    # Description Label
    description_label = tk.Label(root, font=('Comic Sans MS', 12), bg='white', wraplength=640, justify=tk.CENTER)
    description_label.place(relx=0.5, y=600, anchor=tk.N)

    # Like Button and Count
    like_button = tk.Button(root, text="Like", command=toggle_like, width=8, height=2, bg='black', fg='white')
    like_button.place(relx=0.5, y=650, anchor=tk.N)
    like_count_label = tk.Label(root, font=('Comic Sans MS', 12), bg='white')
    like_count_label.place(relx=0.5, y=700, anchor=tk.N)

    # New Post Button
    new_post_button = tk.Button(root, text="New Post", command=new_post, width=16, height=2, bg='lightgreen')
    new_post_button.place(relx=0.5, rely=1.0, anchor=tk.S, y=-10)

    # Navigation Buttons
    first_button = tk.Button(root, text="<<", command=first_post, width=8, height=2, bg='black', fg='white')
    first_button.place(x=90, y=200, anchor=tk.CENTER)

    last_button = tk.Button(root, text=">>", command=last_post, width=8, height=2, bg='black', fg='white')
    last_button.place(x=1100, y=200, anchor=tk.CENTER)

    prev_button = tk.Button(root, text="<", command=previous_post, width=8, height=2, bg='red')
    prev_button.place(x=190, y=450, anchor=tk.CENTER)

    next_button = tk.Button(root, text=">", command=next_post, width=8, height=2, bg='blue')
    next_button.place(x=1000, y=450, anchor=tk.CENTER)

    show_post(currentPostIndex)

    if option == 1:
        refresh_posts()

    root.mainloop()

def create_post_screen(user, client_sock):
    uploaded_image = None
    original_image = None
    current_filter = None

    def upload_image():
        nonlocal uploaded_image, original_image
        file_path = filedialog.askopenfilename(title="Open Image File", filetypes=[("Image files", "*.png *.jpg *.jpeg *.gif *.bmp *.ico")])
        if file_path:
            image = Image.open(file_path)
            img_width, img_height = image.size
            max_width = 640
            max_height = 480

            # Calculate resizing factor to maintain aspect ratio
            width_ratio = max_width / img_width
            height_ratio = max_height / img_height
            resize_ratio = min(width_ratio, height_ratio)

            # Resize image while maintaining aspect ratio
            new_width = int(img_width * resize_ratio)
            new_height = int(img_height * resize_ratio)
            image = image.resize((new_width, new_height), Image.Resampling.LANCZOS)

            # Convert image to PhotoImage format
            img = ImageTk.PhotoImage(image)

            # Update the image label
            image_label.config(image=img, width=new_width, height=new_height)
            image_label.image = img

            # Store the uploaded image
            uploaded_image = cv2.cvtColor(np.array(image), cv2.COLOR_RGB2BGR)
            original_image = uploaded_image.copy() 
            update_reset_button_state()

    def apply_filter(filter_index):
        nonlocal uploaded_image, current_filter, original_image

        if uploaded_image is not None:
            # Convert the image to bytes
            image_bytes = cv2.imencode('.jpg', original_image)[1].tobytes()

            # Send filter signal 
            client_sock.sendall(b'F')

            # Send the filter index (from 0 to 7)
            client_sock.sendall(struct.pack('i', filter_index))

            # Define a fixed-size data type to match size_t on the C side
            size_t_type = ctypes.c_ulong

            # Convert the image size to a ctypes variable
            image_size_ctypes = size_t_type(len(image_bytes))

            client_sock.sendall(image_size_ctypes)

            # Send the image bytes
            client_sock.sendall(image_bytes)

            # Receive the image size
            size_to_receive = ctypes.sizeof(size_t_type)
            buffer = bytearray(size_to_receive)
            received_data = client_sock.recv(size_to_receive)
            buffer[:size_to_receive] = received_data
            received_size_ctypes = size_t_type.from_buffer(buffer)

            print(f"image size in filter: {received_size_ctypes.value}")

            # Receive processed image bytes from server
            processed_image_bytes = b''
            while len(processed_image_bytes) < received_size_ctypes.value:
                packet = client_sock.recv(4096)
                if not packet:
                    break
                processed_image_bytes += packet

            # Decode the image
            processed_image_array = np.frombuffer(processed_image_bytes, dtype=np.uint8)
            filtered_image = cv2.imdecode(processed_image_array, cv2.IMREAD_COLOR)
            filtered_image = cv2.cvtColor(filtered_image, cv2.COLOR_BGR2RGB)

            uploaded_image = filtered_image

            # Update the image in the GUI
            img = ImageTk.PhotoImage(Image.fromarray(filtered_image))
            image_label.config(image=img)
            image_label.image = img
            current_filter = filter_index

    def cancel_action():
        root.destroy()
        create_main_screen(user, posts, client_sock, 1)

    def reset_action():
        nonlocal uploaded_image, current_filter, original_image
        if uploaded_image is not None:
            uploaded_image = original_image.copy()

            img_pil = Image.fromarray(cv2.cvtColor(uploaded_image, cv2.COLOR_BGR2RGB))
            img_tk = ImageTk.PhotoImage(image=img_pil)
            image_label.config(image=img_tk)
            image_label.image = img_tk
            current_filter = None

    def send_post():
        nonlocal uploaded_image, current_filter  # Declare uploaded_image as non-local
        SIGNAL_POST = b"P"
        CHUNK_SIZE = 1024

        description = post_desc_entry.get()

        if not description or uploaded_image is None:
            display_send_error()
            return

        print("Actual send initiated")

        # Send signal POST
        client_sock.sendall(SIGNAL_POST)

        print("sent post signal")

        # Send description
        padded_description = description.ljust(105, '\0')

        # Send the padded description
        client_sock.sendall(padded_description.encode('utf-8'))

        print("sent description:", padded_description)

        uploaded_image_bytes = cv2.imencode('.jpg', uploaded_image)[1].tobytes()
        uploaded_image_size = len(uploaded_image_bytes)

        print(f"image size before: {uploaded_image_size}")

        # Send image size
        image_size_packed = struct.pack('>Q', uploaded_image_size)
        print("Packed image size:", image_size_packed)

        # Send image size
        client_sock.sendall(image_size_packed)

        # Send image data in chunks
        bytes_sent = 0
        while bytes_sent < uploaded_image_size:
            bytes_to_send = min(CHUNK_SIZE, uploaded_image_size - bytes_sent)
            sent = client_sock.send(uploaded_image_bytes[bytes_sent:bytes_sent + bytes_to_send])
            if sent < 0:
                print("Failed to send image data")
                break
            bytes_sent += sent

        print("sent picture")

        root.destroy()
        create_main_screen(user, posts, client_sock, 1)

    def display_send_error():
        messagebox.showerror("Error", "Please enter a description and upload an image.")

    root = tk.Tk()
    root.title("Post Screen")
    root.geometry("800x1200")
    root.configure(bg='white')

    left_frame = tk.Frame(root, bg='white')
    left_frame.place(relx=0, rely=0, relwidth=0.5, relheight=1)

    cancel_button = tk.Button(left_frame, text="Cancel", bg='red', command=cancel_action)
    cancel_button.place(x=10, y=10)

    upload_label = tk.Label(left_frame, text="Upload an image:", bg='white')
    upload_label.place(relx=0.5, y=60, anchor='n')

    upload_button = tk.Button(left_frame, text="Upload", bg='blue', command=upload_image)
    upload_button.place(relx=0.5, y=160, anchor='n')

    post_desc_label = tk.Label(left_frame, text="Post Description:", bg='white')
    post_desc_label.place(x=10, y=200)

    post_desc_entry = tk.Entry(left_frame)
    post_desc_entry.place(x=10, y=230, relwidth=0.9)

    button_names = ["Negative", "Sepia", "Black & White", "Blur", "Cartoon", "Pencil Sketch", "Thermal Vision",
                    "Edge Detection"]

    for i, name in enumerate(button_names):
        button = tk.Button(left_frame, text=name, command=lambda i=i: apply_filter(i))
        button.place(x=10 + (i % 4) * 100, y=280 + (i // 4) * 50)

    right_frame = tk.Frame(root, bg='white')
    right_frame.place(relx=0.5, rely=0, relwidth=0.5, relheight=1)

    # Ensure the image_label can expand to display the image properly
    image_label = tk.Label(right_frame, bg='white')
    image_label.place(relx=0.5, rely=0.1, anchor='n')

    reset_button = tk.Button(right_frame, text="Reset", bg='green', command=reset_action)
    reset_button.place(relx=0.5, y=740, anchor='n')

    send_button = tk.Button(root, text="Send", bg="yellow", command=send_post)
    send_button.place(relx=0.5, rely=0.7, anchor='center')

    def update_reset_button_state():
        if uploaded_image is not None:
            reset_button.config(state=tk.NORMAL)
        else:
            reset_button.config(state=tk.DISABLED)

    root.mainloop()
