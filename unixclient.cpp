#include <sys/socket.h>
#include <sys/un.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#define ADMIN_SOCK "/tmp/admin.sock"

int main() {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, ADMIN_SOCK);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        
        perror("connect - serverul admin nu e pornit?");
        return 1;
    }

    printf("[ADMIN] Conectat la server admin.\n");
    printf("Comenzi disponibile:\n");
    printf("  LIST          - vezi clientii conectati\n");
    printf("  KICK:<fd>     - deconecteaza un client (ex: KICK:5)\n");
    printf("  SHUTDOWN      - opreste serverul\n");
    printf("  EXIT          - iesi din admin\n");

    char input[256];
    char response[4096];

    while (1) {
        printf("[ADMIN]> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "exit") == 0) break;

        // trimite comanda la server
        send(sock, input, strlen(input), 0); 

        // primeste raspuns
        int n = recv(sock, response, sizeof(response) - 1, 0);
        if (n <= 0) {
            printf("[ADMIN] Conexiune inchisa de server.\n");
            break;
        }
        response[n] = '\0';
        printf("%s\n", response);

        // daca serverul s-a oprit, iesim
        if (strcmp(input, "SHUTDOWN") == 0) break;
    }

    close(sock);
    printf("[ADMIN] Deconectat.\n");
    return 0;
}
