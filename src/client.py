import socket
import interface
import struct

class User:
    def __init__(self, user_id, name):
        self.id = user_id
        self.name = name

class Post:
    def __init__(self, post_id, user_id, image, description, user_name, like_count, liked):
        self.id = post_id
        self.userId = user_id
        self.image = image
        self.description = description
        self.userName = user_name
        self.likeCount = like_count
        self.liked = liked

def receive_all(sock, n):
    data = b''
    while len(data) < n:
        packet = sock.recv(n - len(data))
        if not packet:
            return None
        data += packet
    return data

def receive_posts(client_socket):
    # Receive the number of posts
    num_posts_data = receive_all(client_socket, 4)
    num_posts = struct.unpack('I', num_posts_data)[0]
    print(f"Received number of posts: {num_posts}")

    # Receive image sizes
    image_sizes_data = receive_all(client_socket, 8 * num_posts)
    image_sizes = struct.unpack(f'{num_posts}Q', image_sizes_data)
    print(f"Received image sizes: {image_sizes}")

    posts = []
    for i in range(num_posts):
        post_id_data = receive_all(client_socket, 4)
        post_id = struct.unpack('I', post_id_data)[0]
        print(f"Received post ID ({i+1}/{num_posts}): {post_id}")

        user_id_data = receive_all(client_socket, 4)
        user_id = struct.unpack('I', user_id_data)[0]
        print(f"Received user ID ({i+1}/{num_posts}): {user_id}")

        image_size_data = receive_all(client_socket, 8)
        image_size = struct.unpack('Q', image_size_data)[0]
        print(f"Received image size ({i+1}/{num_posts}): {image_size}")

        print("before receiving picture")
        image_data_hex = receive_all(client_socket, image_size)
        print("received image hex ok")

        if len(image_data_hex) != image_size:
            print("inside first if")
            image_data_hex = image_data_hex[:image_size]
            print("new image data hex clamped")

        hex_end = len(image_data_hex)
        print("hex end defined")
        for j in range(len(image_data_hex) - 1, -1, -1):
            print(f"in for: {j}")
            if chr(image_data_hex[j]).lower() in '0123456789abcdef':
                print("if chr")
                hex_end = j + 1
                print("hex end higher")
                break

        image_data_hex = image_data_hex[:hex_end]
        print("image data hex set again, with new hex end")

        try:
            image_data = transform_data(image_data_hex.decode('ascii'))
            print("image data now ascii")
        except ValueError as ve:
            print(f"ValueError while transforming data: {ve}")
            continue

        if len(image_data) != image_size:
            print("second if")
            image_data = image_data[:image_size]

        print("after picture received")

        desc_len_data = receive_all(client_socket, 4)
        desc_len = struct.unpack('I', desc_len_data)[0]
        print(f"Received description length ({i+1}/{num_posts}): {desc_len}")

        description_data = receive_all(client_socket, desc_len)
        description = description_data.decode('utf-8')
        print(f"Received description ({i+1}/{num_posts}): {description}")

        username_len_data = receive_all(client_socket, 4)
        username_len = struct.unpack('I', username_len_data)[0]
        print(f"Received username length ({i+1}/{num_posts}): {username_len}")

        username_data = receive_all(client_socket, username_len)
        username = username_data.decode('utf-8')
        print(f"Received username ({i+1}/{num_posts}): {username}")

        like_count_data = receive_all(client_socket, 4)
        like_count = struct.unpack('I', like_count_data)[0]
        print(f"Received like count ({i+1}/{num_posts}): {like_count}")

        liked_data = receive_all(client_socket, 4)
        liked = bool(liked_data[0])
        print(f"Received liked status ({i+1}/{num_posts}): {liked}")

        posts.append(Post(post_id, user_id, image_data, description, username, like_count, liked))

    return posts


def receive_user_info(client_socket):
    # Receive user ID
    user_id_data = receive_all(client_socket, 4)
    user_id = struct.unpack('I', user_id_data)[0]
    print(f"Received user id: {user_id}")

    # Receive username
    username_data = receive_all(client_socket, 100)
    username = username_data.split(b'\x00', 1)[0].decode('utf-8')
    print(f"Received username: {username}")

    return User(user_id, username)


def hex_char_to_byte(hex_char):
    if '0' <= hex_char <= '9':
        return ord(hex_char) - ord('0')
    elif 'a' <= hex_char <= 'f':
        return ord(hex_char) - ord('a') + 10
    elif 'A' <= hex_char <= 'F':
        return ord(hex_char) - ord('A') + 10
    else:
        raise ValueError(f"Invalid hex character: {hex_char}")

def transform_data(hex_data):
    output_data = bytearray()
    for i in range(2, len(hex_data), 2):
        if i == len(hex_data) - 1:
            byte = hex_char_to_byte(hex_data[i]) << 4
        else:
            byte = (hex_char_to_byte(hex_data[i]) << 4) | hex_char_to_byte(hex_data[i + 1])
        output_data.append(byte)
    return bytes(output_data)


def main():
    server_ip = "127.0.0.1"
    server_port = 8080

    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        client_socket.connect((server_ip, server_port))
        print("Connected to the server.")

        interface.create_login_screen(client_socket)

        user = receive_user_info(client_socket)

        posts = receive_posts(client_socket)
        interface.create_main_screen(user, posts, client_socket)

    except Exception as e:
        print("Error:", e)

    finally:
        client_socket.close()
        print("Socket closed.")

if __name__ == "__main__":
    main()
