#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 24
#define MAX_CLIENTS 10000
#define BUFFER_SIZE 1000

struct Client {
    int socket_fd;
    // Add other fields you might need for each client
};

int main() {
    int server_socket, max_sd, activity;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);
    fd_set readfds;
    struct Client clients[MAX_CLIENTS];
    char buffer[BUFFER_SIZE];

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket_fd = 0;
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Binding failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("Listening failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        max_sd = server_socket;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_socket = clients[i].socket_fd;

            if (client_socket > 0) {
                FD_SET(client_socket, &readfds);
            }

            if (client_socket > max_sd) {
                max_sd = client_socket;
            }
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("Select error");
        }

        if (FD_ISSET(server_socket, &readfds)) {
            if (max_sd < MAX_CLIENTS - 1) {
                if ((clients[max_sd + 1].socket_fd = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size)) != -1) {
                    printf("New connection, socket fd is %d\n", clients[max_sd + 1].socket_fd);
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_socket = clients[i].socket_fd;

            if (FD_ISSET(client_socket, &readfds)) {
                int valread;
                if ((valread = read(client_socket, buffer, BUFFER_SIZE)) == 0) {
                    printf("Host disconnected, socket fd is %d\n", client_socket);
                    close(client_socket);
                    clients[i].socket_fd = 0;
                } else {
                    buffer[valread] = '\0';
                    printf("Client Sent: %s\n", buffer);

                    if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
                        perror("Error sending response");
                    }
                }
            }
        }
    }

    close(server_socket);
    return 0;
}

