#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 24
#define MAX_EVENTS 4000
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
    int epoll_fd, event_count;
    struct epoll_event event, events[MAX_EVENTS];
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options for reusing addresses and ports
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        perror("Setsockopt failed");
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
    if (listen(server_socket, SOMAXCONN) == -1) {
        perror("Listening failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    // Create epoll instance
    if ((epoll_fd = epoll_create1(0)) == -1) {
        perror("Epoll create failed");
        exit(EXIT_FAILURE);
    }

    event.events = EPOLLIN;
    event.data.fd = server_socket;

    // Add server socket to epoll instance
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1) {
        perror("Epoll control failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < event_count; ++i) {
            if (events[i].data.fd == server_socket) {
                if ((new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size)) != -1) {
                    printf("New connection, socket fd is %d\n", new_socket);

                    // Set the new client socket to non-blocking
                    int flags = fcntl(new_socket, F_GETFL, 0);
                    fcntl(new_socket, F_SETFL, flags | O_NONBLOCK);

                    event.events = EPOLLIN | EPOLLET; // Edge-triggered
                    event.data.fd = new_socket;

                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &event) == -1) {
                        perror("Epoll control failed for client socket");
                        close(new_socket);
                    }
                }
            } else {
                int socket_fd = events[i].data.fd;
                int valread;

                // Keep reading until all available data is read from the socket
                while ((valread = read(socket_fd, buffer, BUFFER_SIZE)) > 0) {
                    buffer[valread] = '\0';
                    printf("Client Sent: %s\n", buffer);

                    // Process incoming data and generate a response
                    // ... (Your data processing logic)

                    // Send response to the client
                    if (send(socket_fd, buffer, valread, 0) == -1) {
                        perror("Error sending response");
                    }
                }

                // Handling read, closure, or error cases
                if (valread == 0) {
                    printf("Host disconnected, socket fd is %d\n", socket_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socket_fd, NULL);
                    close(socket_fd);
                } else if (valread == -1) {
                    if (errno != EAGAIN) {
                        perror("Error while reading");
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socket_fd, NULL);
                        close(socket_fd);
                    }
                }
            }
        }
    }

    // Clean-up and close socket descriptors
    close(server_socket);
    close(epoll_fd);
    return 0;
}

