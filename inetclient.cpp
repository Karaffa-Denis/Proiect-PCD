#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <vector>

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
    int64_t size = get_file_size(f);
    send(sock, &size, sizeof(size), 0);
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        send(sock, buf, n, 0);
    fclose(f);
    printf("[CLIENT] Imagine trimisa: %s (%ld bytes)\n", path, size);

    // primeste imaginea procesata
    int64_t out_size = 0;
    recv(sock, &out_size, sizeof(out_size), 0);

    std::vector<unsigned char> result(out_size);
    ssize_t total = 0;
    while (total < out_size) {
        ssize_t r = recv(sock, result.data() + total, out_size - total, 0);
        if (r <= 0) break;
        total += r;
    }

    FILE* out = fopen("rezultat.jpg", "wb");
    fwrite(result.data(), 1, out_size, out);
    fclose(out);
    printf("[CLIENT] Imagine procesata salvata: rezultat.jpg (%ld bytes)\n", out_size);
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
    printf("Comenzi: scrie un mesaj, 'SEND:<cale>' pentru imagine, 'EXIT' pentru iesire\n\n");

    char input[512];
    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;

        // sterge newline
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "EXIT") == 0) break;

        // trimitere imagine
        if (strncmp(input, "SEND:", 5) == 0) {
            send_file(sock, input + 5);
            continue;
        }
    }

    close(sock);
    printf("[CLIENT] Deconectat.\n");
    return 0;
}
