#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>

#define PORT 24
#define MAX_CLIENTS 150
#define BUFFER_SIZE 1000

unsigned long long computeFactorial(unsigned long n) {
    unsigned long long fact = 1;
    if (n > 20) {
        n = 20; // Limit the number to prevent overflow
    }

    for (unsigned long i = 1; i <= n; i++) {
        fact *= i;
    }

    return fact;
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);
    struct pollfd fds[MAX_CLIENTS];
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Binding failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 10) == -1) {
        perror("Listening failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    fds[0].fd = server_socket;
    fds[0].events = POLLIN;

    for (int i = 1; i < MAX_CLIENTS; i++) {
        fds[i].fd = -1; // Initialize with -1 to indicate an available slot
    }

    while (1) {
        int activity = poll(fds, MAX_CLIENTS, -1);

        if (activity < 0) {
            perror("Poll error");
            exit(EXIT_FAILURE);
        }

        if (fds[0].revents & POLLIN) {
            if ((new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size)) != -1) {
                printf("New connection, socket fd is %d\n", new_socket);

                int i;
                for (i = 1; i < MAX_CLIENTS; i++) {
                    if (fds[i].fd == -1) {
                        fds[i].fd = new_socket;
                        fds[i].events = POLLIN;
                        break;
                    }
                }

                if (i == MAX_CLIENTS) {
                    printf("Too many clients\n");
                    close(new_socket);
                }
            }
        }

        for (int i = 1; i < MAX_CLIENTS; i++) {
            if (fds[i].fd != -1 && fds[i].revents & POLLIN) {
                int socket_fd = fds[i].fd;
                int valread = read(socket_fd, buffer, BUFFER_SIZE);

                if (valread == 0) {
                    printf("Host disconnected, socket fd is %d\n", socket_fd);
                    close(socket_fd);
                    fds[i].fd = -1;
                } else if (valread > 0) {
                    buffer[valread] = '\0';
                    printf("Client Sent: %s\n", buffer);

                    unsigned long n = strtoul(buffer, NULL, 10);
                    unsigned long long result = computeFactorial(n);

                    char response_buffer[BUFFER_SIZE];
                    int response_length = snprintf(response_buffer, BUFFER_SIZE, "Factorial of %lu is %llu", n, result);

                    if (send(socket_fd, response_buffer, response_length, 0) == -1) {
                        perror("Error sending response");
                    }
                }
            }
        }
    }

    close(server_socket);
    return 0;
}

