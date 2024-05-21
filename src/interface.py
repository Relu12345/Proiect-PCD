import sys
import tkinter as tk
from tkinter import messagebox
import cv2
import numpy as np
import io
from PIL import Image, ImageTk

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


currentPostIndex = 0


def create_main_screen(user, posts):
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
        print("Refreshing posts...")

    def new_post():
        print("Creating new post...")

    def toggle_like():
        post = posts[currentPostIndex]
        if post.liked:
            post.likeCount -= 1
        else:
            post.likeCount += 1
        post.liked = not post.liked
        show_post(currentPostIndex)

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
    description_label.place(relx=0.5, y=530, anchor=tk.N)

    # Like Button and Count
    like_button = tk.Button(root, text="Like", command=toggle_like, width=8, height=2, bg='black', fg='white')
    like_button.place(relx=0.5, y=600, anchor=tk.N)
    like_count_label = tk.Label(root, font=('Comic Sans MS', 12), bg='white')
    like_count_label.place(relx=0.5, y=650, anchor=tk.N)

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

    root.mainloop()