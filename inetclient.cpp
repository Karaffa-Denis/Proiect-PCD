#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"

long get_file_size(FILE* f) {
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    return size;
}

void send_file(int sock, const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { perror("fopen"); return; }
    char buf[65536];
    size_t n;
    long size = get_file_size(f);
    send(sock, &size, sizeof(size), 0);
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        send(sock, buf, n, 0);
    fclose(f);
    printf("[CLIENT] Imagine trimisa: %s (%ld bytes)\n", path, size); 
}

void receive_file(int sock, const char* out_path) {
    long out_size = 0; // Presupunem că serverul trimite tot un 'long' nativ
    
    //Citim dimensiunea imaginii
    ssize_t n = recv(sock, &out_size, sizeof(out_size), 0);
    if (n <= 0) {
        if (n == 0) {
            printf("[CLIENT] Deconectat de la server (KICK)\n");
        } else {
            perror("[ERR] Eroare la primirea dimensiunii");
        }
        return;
    }

    printf("[CLIENT] Se primeste imaginea procesata (%ld bytes)...\n", out_size);

    FILE* f = fopen(out_path, "wb");
    if (!f) { 
        perror("[ERR] Eroare la deschiderea fisierului de output"); 
        return; 
    }

    //Citirea fluxului de date
    char buf[65536];
    long total_received = 0;

    while (total_received < out_size) {
        long remaining = out_size - total_received;
        
        size_t to_read = (remaining < (long)sizeof(buf)) ? remaining : sizeof(buf);
        
        ssize_t bytes_read = recv(sock, buf, to_read, 0);
        
        if (bytes_read < 0) {
            perror("[ERR] Eroare la citirea octetilor din retea");
            break;
        } else if (bytes_read == 0) {
            printf("[CLIENT] Conexiunea a fost inchisa neasteptat de server in timpul transferului.\n");
            break;
        }
        
        fwrite(buf, 1, bytes_read, f);
        total_received += bytes_read;
    }

    fclose(f);

    if (total_received == out_size) {
        printf("[CLIENT] Imaginea procesata a fost salvata cu succes in: %s\n", out_path);
    } else {
        printf("[ERR] Transfer incomplet. Primit: %ld / %ld bytes.\n", total_received, out_size);
    }
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }
    printf("[CLIENT] Conectat la %s:%d\n", SERVER_IP, PORT);
    printf("Comenzi:'SEND:<cale>' pentru imagine, 'exit' pentru iesire\n\n");

    char input[512];
    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;

        // sterge newline
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "exit") == 0) break;

        // trimitere imagine
        if (strncmp(input, "SEND:", 5) == 0) {
            send_file(sock, input + 5);
            receive_file(sock, "output_procesat.jpg"); 
            continue;
        }

        char response[512];
        int n = recv(sock, response, sizeof(response) - 1, 0);
        if (n == 0) {
            printf("[CLIENT] Am fost dat afara de pe server (KICK)!\n");
            break;
        }
        if (n > 0) {
            response[n] = '\0';
            printf("[SERVER] %s\n", response);
        }
    }

    close(sock);
    printf("[CLIENT] Deconectat.\n");
    return 0;
}
