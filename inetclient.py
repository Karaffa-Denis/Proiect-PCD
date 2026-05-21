import socket
import struct
import os

PORT = 8080
SERVER_IP = "127.0.0.1"

def send_file(sock, path):
    try:
        # obtinerea dimensiunii fisierului
        file_size = os.path.getsize(path)

        # marimea fisierului
        size_bytes = struct.pack('@l', file_size)
        sock.sendall(size_bytes)

        # deschidere si trimitere secventiala a fisierului
        with open(path, 'rb') as f:
            while True:
                chunk = f.read(65536)
                if not chunk:
                    break
                sock.sendall(chunk)

        print(f"[CLIENT] Imagine trimisa: {path} ({file_size} bytes)")

    # diverse erori
    except FileNotFoundError:
        print(f"Eroare: Fisierul '{path}' nu a putut fi deschis.")
    except Exception as e:
        print(f"Eroare la trimitere: {e}")


def main():
    # creare socket
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((SERVER_IP, PORT))
    except ConnectionRefusedError:
        print(f"Eroare: Nu ma pot conecta la server {SERVER_IP}:{PORT}")
        return

    print(f"[CLIENT] Conectat la {SERVER_IP}:{PORT}")
    print("Comenzi: scrie un mesaj, 'SEND:<cale>' pentru imagine, 'exit' pentru iesire\n")

    while True:
        try:
            # citire input
            user_input = input("> ").strip()
        except (EOFError, KeyboardInterrupt):
            break  # iesire ctrl+c

        if not user_input:
            continue

        if user_input == "exit":
            break

        # trimitere imagine
        if user_input.startswith("SEND:"):
            # path extraction
            file_path = user_input[5:]
            send_file(sock, file_path)
            continue

        #trimitere mesaj
        try:
            # convertire in octeti inaite de trimitrere (specific python)
            sock.sendall(user_input.encode('utf-8'))

            # raspunsul
            response = sock.recv(512)
            if response:
                #decodare
                print(f"[SERVER] {response.decode('utf-8', errors='ignore')}")
            else:
                print("[CLIENT] Conexiune inchisa de server.")
                break
        except Exception as e:
            print(f"[CLIENT] Eroare: {e}")
            break

    sock.close()
    print("[CLIENT] deconectat.")


if __name__ == "__main__":
    main()