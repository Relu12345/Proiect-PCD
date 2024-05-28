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
import filters

currentPostIndex = 0
posts = []

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
    else:
        messagebox.showerror("Login Failed", "Login failed. Please try again.")
        sys.exit(1)

def on_register(client_socket, root):
    send_message(client_socket, "0")
    username = username_entry.get()
    password = password_entry.get()
    message = username + "," + password
    send_message(client_socket, message)

    # Receive response from the server
    response = client_socket.recv(1024).split(b'\x00', 1)[0].decode()
    print("response: " + response)
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
        send_data = SIGNAL_LIKE + struct.pack("ii", post_id, user_id)
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
    current_filter = None

    def upload_image():
        nonlocal uploaded_image
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

    def apply_filter(filter_function):
        nonlocal uploaded_image, current_filter
        if uploaded_image is not None:
            filtered_image = filter_function(uploaded_image)
            filtered_image = cv2.cvtColor(filtered_image, cv2.COLOR_BGR2RGB)
            img = ImageTk.PhotoImage(Image.fromarray(filtered_image))
            image_label.config(image=img)
            image_label.image = img
            current_filter = filter_function

    def cancel_action():
        root.destroy()
        create_main_screen(user, posts, client_sock, 1)

    def reset_action():
        nonlocal uploaded_image, current_filter
        if uploaded_image is not None:
            img = ImageTk.PhotoImage(Image.fromarray(cv2.cvtColor(uploaded_image, cv2.COLOR_BGR2RGB)))
            image_label.config(image=img)
            image_label.image = img
            current_filter = None

    def send_post():
        nonlocal uploaded_image  # Declare uploaded_image as non-local
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
        description_bytes = description.encode('utf-8')
        client_sock.sendall(description_bytes)

        print("sent description:", description)

        # Apply current filter if any
        if current_filter:
            uploaded_image = current_filter(uploaded_image)

        # Encode image
        _, buffer = cv2.imencode('.jpg', uploaded_image)

        uploaded_image_bytes = cv2.imencode('.jpg', uploaded_image)[1].tobytes()
        uploaded_image_size = len(uploaded_image_bytes)
        size_bytes = struct.pack("Q", uploaded_image_size)

        # Send image size
        client_sock.sendall(size_bytes)

        # Send image data in chunks
        bytes_sent = 0
        while bytes_sent < uploaded_image_size:
            bytes_to_send = min(CHUNK_SIZE, uploaded_image_size - bytes_sent)
            sent = client_sock.send(uploaded_image_bytes[bytes_sent:bytes_sent + bytes_to_send])
            if sent < 0:
                print("Failed to send image data")
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

    def apply_filter(filter_function):
        nonlocal uploaded_image, current_filter
        if uploaded_image is not None:
            filtered_image = filter_function(uploaded_image)
            filtered_image = cv2.cvtColor(filtered_image, cv2.COLOR_BGR2RGB)
            img = ImageTk.PhotoImage(Image.fromarray(filtered_image))
            image_label.config(image=img)
            image_label.image = img
            current_filter = filter_function

    filter_functions = [filters.applyNegative, filters.applySepia, filters.applyBlackAndWhite, filters.applyBlur,
                        filters.applyCartoonEffect, filters.applyPencilSketch, filters.applyThermalVision,
                        filters.applyEdgeDetection]

    button_names = ["Negative", "Sepia", "Black & White", "Blur", "Cartoon", "Pencil Sketch", "Thermal Vision",
                    "Edge Detection"]

    for i, (name, func) in enumerate(zip(button_names, filter_functions)):
        button = tk.Button(left_frame, text=name, command=lambda f=func: apply_filter(f))
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

    root.mainloop()
