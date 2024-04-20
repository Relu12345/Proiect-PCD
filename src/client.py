import socket

SERVER_ADDRESS = ('localhost', 8080)

def main():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        client_socket.connect(SERVER_ADDRESS)
        print("Connected to server.")

        while True:
            message = input("Enter message: ")
            if not message:
                break

            client_socket.sendall(message.encode())

    except Exception as e:
        print(f"Error: {e}")
    finally:
        client_socket.close()


if __name__ == "__main__":
    main()
