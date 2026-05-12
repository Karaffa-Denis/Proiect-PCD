#ifndef PROTO_H
#define PROTO_H

#define MAX_CLIENTS 64

void *unix_main(void* args) ;
void *inet_main(void* args) ;
void remove_client(int fd); 

extern int shutdown_in_progress;
extern int connected_clients[MAX_CLIENTS];
extern int num_clients;
extern int master_unix_fd;
extern int master_inet_fd;

#endif