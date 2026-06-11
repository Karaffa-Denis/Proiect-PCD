import socket
import struct
import os

PORT = 8080
SERVER_IP = "127.0.0.1"

def send_file(sock, path):
    try:
        file_size = os.path.getsize(path)

        # '<q' = int64_t, 8 bytes, little-endian
        size_bytes = struct.pack('<q', file_size)
        sock.sendall(size_bytes)

        with open(path, 'rb') as f:
            while True:
                chunk = f.read(65536)
                if not chunk:
                    break
                sock.sendall(chunk)

        print(f"[CLIENT] Imagine trimisa: {path} ({file_size} bytes)")

        # primeste imaginea procesata
        out_size_bytes = sock.recv(8)
        out_size = struct.unpack('<q', out_size_bytes)[0]

        result = b''
        while len(result) < out_size:
            chunk = sock.recv(min(65536, out_size - len(result)))
            if not chunk:
                break
            result += chunk

        with open('rezultat.jpg', 'wb') as f:
            f.write(result)

        print(f"[CLIENT] Imagine procesata salvata: rezultat.jpg ({out_size} bytes)")

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

        
        
        sock.sendall(user_input.encode('utf-8'))

            
        

    sock.close()
    print("[CLIENT] deconectat.")


if __name__ == "__main__":
    main()
