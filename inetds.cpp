#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <sys/un.h>
#include <string.h> 
#include <pthread.h>
#include <opencv2/opencv.hpp>

#include "proto.h"



#define ADDR "127.0.0.1"
#define PORT 8080



void add_client(int fd);
void remove_client(int fd);
void *handle_client(void* arg);

void *inet_main (void* args) {
    
    master_inet_fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("[INFO] INET thread started at fd %d\n", master_inet_fd);
    int opt = 1;
    setsockopt(master_inet_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ADDR);
    addr.sin_port = htons(PORT);


    //Incercam bind si listen pe socket-ul creat
    int bind_ret = bind(master_inet_fd, (struct sockaddr *) &addr, sizeof(addr));
    if (bind_ret < 0)
    {
        perror("Error on binding socket");
        exit(EXIT_FAILURE);
    }

    if (listen(master_inet_fd, MAX_CLIENTS) < 0)
    {
        perror("Error on listen");
        exit(EXIT_FAILURE);

    }

    while(shutdown_in_progress == 0){
        int *client_fd = new int(accept(master_inet_fd, NULL, NULL));

        add_client(*client_fd);

        printf("Client cu fd:%d acceptat\n", *client_fd);

        //cream thread detached pt fiecare client
        pthread_t thread;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        pthread_create(&thread, &attr, handle_client, client_fd);

        //eliberam memoria
        pthread_attr_destroy(&attr);
        }
    
    close(master_inet_fd);
    return nullptr;

}

void *handle_client(void* arg){
    int client_fd = *(int*) arg;
    delete arg;
    
    //apelare procesare opencv
    char buffer[1024];
    long file_size = 0;

    // primim dimensionea
    if (recv(client_fd, &file_size, sizeof(file_size), 0) <= 0) {
        close(client_fd);
        return NULL;
    }

    // citire date imagine
    std::vector<uchar> data(file_size);
    ssize_t total_received = 0;
    while (total_received < file_size) {
        ssize_t n = recv(client_fd, data.data() + total_received, file_size - total_received, 0);
        if (n <= 0) break;
        total_received += n;
    }

    // citire opencv
    cv::Mat img = cv::imdecode(data, cv::IMREAD_COLOR);
    if (img.empty()) {
        char err[] = "eroare la procesare";
        send(client_fd, err, strlen(err), 0);
        close(client_fd);
        return NULL;
    }

    // redimensionare imagine
    cv::Mat gray, binary;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    cv::adaptiveThreshold(gray, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 31, 15);
    
    std::vector<cv::Point> points;
    cv::findNonZero(binary, points);
    
    cv::Mat processed;
    if (!points.empty()) {
        cv::Rect textRect = cv::boundingRect(points);
        int padding = 40; // Hack-ul tău de padding
        textRect.x = std::max(0, textRect.x - padding);
        textRect.y = std::max(0, textRect.y - padding);
        textRect.width = std::min(img.cols - textRect.x, textRect.width + 2 * padding);
        textRect.height = std::min(img.rows - textRect.y, textRect.height + 2 * padding);
        
        cv::Mat finalScan;
        cv::adaptiveThreshold(gray, finalScan, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 31, 15);
        processed = finalScan(textRect);
    } else {
        processed = gray; // fallback
    }

    //reencodare jpg pentru trimitere
    std::vector<uchar> buf_out;
    cv::imencode(".jpg", processed, buf_out);
    long out_size = buf_out.size();


    // transformam imaginea in jpg
    std::vector<uchar> buffer_out;
    cv::imencode(".jpg", cropped, buffer_out);

    // noua dimensiune
    long out_size = buffer_out.size();
    send(client_fd, &out_size, sizeof(out_size), 0);

    // trimitem datele
    ssize_t sent_bytes = send(client_fd, buffer_out.data(), out_size, 0);

    if (sent_bytes > 0) {
        printf("[SERVER] am trimis imaginea procesata inapoi (%ld bytes)\n", out_size);
    }

    close(client_fd);
    remove_client(client_fd);

    return NULL;
}

void add_client(int fd){
    if (num_clients < MAX_CLIENTS)
        connected_clients[num_clients++] = fd;
}
