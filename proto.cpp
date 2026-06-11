#include "proto.h"
#include <pthread.h>
// Definirea variabilelor (se face o singură dată aici)
int shutdown_in_progress = 0;
int connected_clients[MAX_CLIENTS];
int num_clients = 0;
int master_unix_fd;
int master_inet_fd;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

int remove_client(int fd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < num_clients; i++) {
        if (connected_clients[i] == fd) {
            for (int j = i; j < num_clients - 1; j++) {
                connected_clients[j] = connected_clients[j + 1];
            }
            num_clients--;
            pthread_mutex_unlock(&clients_mutex);
            return 1;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return 0;
}