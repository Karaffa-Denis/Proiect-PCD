#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <sys/un.h>
#include <string.h> 
#include <pthread.h>
#include <opencv2/opencv.hpp>
#include <algorithm> 
#include <numeric>
#include "proto.h"

#define ADDR "127.0.0.1"
#define PORT 8080

void add_client(int fd);
void *handle_client(void* arg);
std::vector<cv::Point2f> order_points(const std::vector<cv::Point>& pts) {
    std::vector<cv::Point2f> rect(4);
    std::vector<int> sum(4), diff(4);

    for (int i = 0; i < 4; i++) {
        sum[i] = pts[i].x + pts[i].y;
        diff[i] = pts[i].y - pts[i].x;
    }

    // stanga sus
    rect[0] = pts[std::distance(sum.begin(), std::min_element(sum.begin(), sum.end()))];
    // dreapta sus
    rect[1] = pts[std::distance(diff.begin(), std::min_element(diff.begin(), diff.end()))];
    // dreapta-jos 
    rect[2] = pts[std::distance(sum.begin(), std::max_element(sum.begin(), sum.end()))];
    // stanga jos
    rect[3] = pts[std::distance(diff.begin(), std::max_element(diff.begin(), diff.end()))];

    return rect;
}


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
        perror("Eroare la bind pe socket");
        exit(EXIT_FAILURE);
    }

    if (listen(master_inet_fd, MAX_CLIENTS) < 0)
    {
        perror("Error la listen");
        exit(EXIT_FAILURE);

    }

    while(shutdown_in_progress == 0){
        int *client_fd = new int(accept(master_inet_fd, NULL, NULL));

        if (*client_fd < 0) {
            if (shutdown_in_progress) {
                break; 
            }
            perror("Eroare la accept");
            continue;
        }

        add_client(*client_fd);

        printf("Client cu fd:%d acceptat\n", *client_fd);

        //Cream thread detached pt fiecare client
        pthread_t thread;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        pthread_create(&thread, &attr, handle_client, client_fd);

        //Eliberam memoria
        pthread_attr_destroy(&attr);
        }
    
    close(master_inet_fd);
    return nullptr;
}

void *handle_client(void* arg){
    int client_fd = *(int*) arg;
    delete (int*)arg;
    
    int64_t file_size = 0;

    // Primim dimensiunea imaginii
    if (recv(client_fd, &file_size, sizeof(file_size), 0) <= 0) {
        close(client_fd);
        remove_client(client_fd);
        return NULL;
    }

    // Citire date imagine în chunk-uri
    std::vector<uchar> data(file_size);
    ssize_t total_received = 0;
    while (total_received < file_size) {
        ssize_t n = recv(client_fd, data.data() + total_received, file_size - total_received, 0);
        if (n <= 0) break;
        total_received += n;
    }

    // Decodare OpenCV
    cv::Mat img = cv::imdecode(data, cv::IMREAD_COLOR);
    if (img.empty()) {
        printf("[SERVER] Imaginea nu a putut fi decodata.\n");
        close(client_fd);
        remove_client(client_fd);
        return NULL;
    }

    // START PROCESARE
    cv::Mat gray, blurred, edged, processed;
    
    //Conversie tonuri de gri
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    
    // Eliminare zgomot
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);
    cv::Canny(blurred, edged, 75, 200);

    // b) Extragerea contururilor
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edged, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    // Sortam contururile dupa arie
    std::sort(contours.begin(), contours.end(), [](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
        return cv::contourArea(a) > cv::contourArea(b);
    });

    std::vector<cv::Point> document_contour;
    // Calculam aria minima acceptabila (10% din imagine)
    double min_area = (double)img.cols * img.rows * 0.1;

    for (const auto& contour : contours) {
        double peri = cv::arcLength(contour, true);
        std::vector<cv::Point> approx;
        cv::approxPolyDP(contour, approx, 0.02 * peri, true);

        // Verificam ca fiecare contur sa aiba 4 colturi si minim min_area
        if (approx.size() == 4 && cv::contourArea(approx) > min_area) {
            document_contour = approx;
            break;
        }
    }

    // Tansformare de perspectiva și Threshold
    if (!document_contour.empty()) { //Daca am gasit contur
        std::vector<cv::Point2f> ordered_corners = order_points(document_contour); 
        
        std::vector<cv::Point2f> destination_corners = {
            cv::Point2f(0, 0),
            cv::Point2f(img.cols - 1, 0),
            cv::Point2f(img.cols - 1, img.rows - 1),
            cv::Point2f(0, img.rows - 1)
        };

        cv::Mat transform_matrix = cv::getPerspectiveTransform(ordered_corners, destination_corners);
        cv::Mat warped_img;
        cv::warpPerspective(gray, warped_img, transform_matrix, cv::Size(img.cols, img.rows));

        // Aplicam un blur inainte de threshhold pt netezire
        cv::GaussianBlur(warped_img, warped_img, cv::Size(3, 3), 0);
        
        // Threshhold
        cv::adaptiveThreshold(warped_img, processed, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 21, 10);
    } else {
        // Daca nu am gasit contur, aplicam doar threshhold-ul
        cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0);
        cv::adaptiveThreshold(gray, processed, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 21, 10);
    }

    // Transformam imaginea inapoi în JPG
    std::vector<uchar> buffer_out;
    cv::imencode(".jpg", processed, buffer_out);

    // Trimitem dimensiunea noua spre client
    int64_t out_size = buffer_out.size();
    send(client_fd, &out_size, sizeof(out_size), 0);

    // Trimitem bytesii imaginii procesate 
    ssize_t sent_bytes = send(client_fd, buffer_out.data(), out_size, 0);

    if (sent_bytes > 0) {
        printf("[SERVER] Am trimis imaginea procesata inapoi (%ld bytes)\n", out_size);
    }

    close(client_fd);
    remove_client(client_fd);

    return NULL;
}

void add_client(int fd){
    if (num_clients < MAX_CLIENTS)
        connected_clients[num_clients++] = fd;
}
