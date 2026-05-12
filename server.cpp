#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <sys/un.h>
#include <pthread.h>
#include "proto.h"

int main() {

    pthread_t unix_thread;
    pthread_t inet_thread;

    pthread_create (&unix_thread, NULL, unix_main, NULL);
    pthread_create (&inet_thread, NULL, inet_main, NULL) ;

    pthread_join (unix_thread, NULL) ;
    pthread_join (inet_thread, NULL) ;
}

