#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <string.h> 
#include <time.h> 
#include <pthread.h>
#include "proto.h"
#define UNIX_SOCK "/tmp/admin.sock"
#define BUFFER_SIZE 1024
#define RESPONSE_SIZE 1024

int admin_connected = 0;
void* handle_admin(void* arg);


void *unix_main (void* args){
    
    unlink(UNIX_SOCK);
    master_unix_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    printf("[INFO] UNIX thread started at fd %d\n", master_unix_fd);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, UNIX_SOCK, sizeof(addr.sun_path) - 1);


    //Incercam bind si listen pe socket-ul creat
    int bind_ret = bind(master_unix_fd, (struct sockaddr *) &addr, sizeof(addr));
    if (bind_ret < 0)
    {
        perror("Error on binding socket");
        exit(EXIT_FAILURE);
    }

    if (listen(master_unix_fd,1) < 0)
    {
        perror("Error on listen");
        exit(EXIT_FAILURE);
    }


    while(shutdown_in_progress==0){
        int *client_fd = new int(accept(master_unix_fd, NULL, NULL));

        if(admin_connected==0){
            printf("[INFO] Client admin cu fd:%d acceptat\n", *client_fd);
            
            admin_connected = 1; 

            pthread_t thread;

            //creem thread detached
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

            pthread_create(&thread, &attr, handle_admin, client_fd);

            //eliberam memoria
            pthread_attr_destroy(&attr);

        }
        else{
            char resp[] = "[INFO] Admin deja conectat\n";
            printf("%s", resp);
            send(*client_fd, resp, strlen(resp),0);
            
            close(*client_fd);
            delete client_fd;
        }

        
    }

    unlink(UNIX_SOCK);
    close(master_unix_fd);
    return nullptr;
}

void* handle_admin(void* arg){
    int client_fd = *(int*) arg;
    delete arg;

    int n;
    char response[RESPONSE_SIZE], buffer[BUFFER_SIZE];

    while(( n = recv(client_fd, buffer, BUFFER_SIZE, 0)) > 0 ){
        buffer[n] = '\0';

        //cazul LIST
        if (strcmp(buffer, "LIST") == 0) {
            printf("::: Sending list of clients to admin :::\n");
            
            int offset = sprintf(response, "Fd-urile clienților conectați (%d total): ", num_clients);

            for (int i = 0; i < num_clients; i++) {
                offset += sprintf(response + offset, "%d%s", connected_clients[i], (i == num_clients - 1) ? "" : ", ");
            }
        
            strcat(response, "\n");
            send(client_fd, response, strlen(response), 0);
        }       


        //cazul KICK
        #define KICKFD 5
        else if(strncmp(buffer, "KICK:", KICKFD) == 0) {
            int kick_fd = atoi(buffer+KICKFD);

            printf("::: Kicking client with fd %d :::\n", kick_fd);

            close(kick_fd); 
            remove_client(kick_fd);

            if( sprintf(response, "Socket-ul clientului cu fd-ul: %d a fost inchis", kick_fd) < 0 ){
                    perror("Eroare sprintf");
                    exit(EXIT_FAILURE);
            }

            send(client_fd, response, strlen(response), 0);
        }

        //cazul SHUTDOWN
        else if(strcmp(buffer, "SHUTDOWN")== 0){
            printf("::: Server shutdown :::\n");
            shutdown_in_progress = 1;
            close(master_unix_fd);
            close(master_inet_fd);
        }

        //cazul EXIT
        else if(strcmp(buffer, "EXIT") == 0){
            printf("::: Admin is exiting :::\n");
            close(client_fd);
            admin_connected = 0; 
        }

    }

    close(client_fd);
    admin_connected = 0; 
    return NULL;
}