#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <arpa/inet.h>

#define PORT 24

sem_t semaphore;

typedef struct {
    int client_socket;
    struct sockaddr_in client_address;
    socklen_t address_length;
} ThreadData;

void *clientfunc(void *args) {
    ThreadData *data = (ThreadData *)args;
    int client_socket = data->client_socket;
    struct sockaddr_in client_address = data->client_address;

    char buffer[1024];
    int bytes_received;

    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        printf("Client: %s\n", buffer);
        send(client_socket, buffer, strlen(buffer), 0);
        bzero(buffer, sizeof(buffer));
    }

    if (bytes_received == 0) {
        char ip_address[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_address.sin_addr), ip_address, INET_ADDRSTRLEN);
        printf("Client disconnected from %s:%d\n", ip_address, ntohs(client_address.sin_port));
        close(client_socket);
    } else {
        perror("Receive error");
    }

    free(data);
    pthread_exit(NULL);
}
int main() {
    int server_fd;
    struct sockaddr_in server_address;
    sem_init(&semaphore, 0, 0);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        ThreadData *data = (ThreadData *)malloc(sizeof(ThreadData));
        int client_socket;
        struct sockaddr_in client_address;
        int address_length = sizeof(client_address);

        client_socket = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&address_length);

        if (client_socket < 0) {
            perror("Accept failed");
            free(data);
            continue;
        }

        data->client_socket = client_socket;
        data->client_address = client_address;
        data->address_length = address_length;

        pthread_t client_thread;
        pthread_create(&client_thread, NULL, clientfunc, (void *)data);
        pthread_detach(client_thread);
    }

    close(server_fd);
    return 0;
}

