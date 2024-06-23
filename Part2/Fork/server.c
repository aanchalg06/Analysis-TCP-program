#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 24

unsigned long long computeFactorial(unsigned long n) {
    unsigned long long fact = 1;
    if (n > 20) {
        n = 20;  // If n is greater than 20, limit it to 20
    }

    for (unsigned long i = 2; i <= n; i++) {
        fact *= i;
    }
    return fact;
}

int main() {
    int sockfd, ret;
    struct sockaddr_in serverAddr;
    int newSocket;
    struct sockaddr_in newAddr;
    socklen_t addr_size = sizeof(newAddr);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation error");
        exit(1);
    }
    printf("[+]Server Socket is created.\n");

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret < 0) {
        perror("Binding error");
        exit(1);
    }
    printf("[+]Bind to port %d\n", PORT);

    if (listen(sockfd, 10) == 0) {
        printf("[+]Listening....\n");
    } else {
        perror("Listening error");
        exit(1);
    }

    while (1) {
        newSocket = accept(sockfd, (struct sockaddr *)&newAddr, &addr_size);
        if (newSocket < 0) {
            perror("Accept error");
            exit(1);
        }
        printf("Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));

        pid_t childpid = fork();
        if (childpid == 0) {
            close(sockfd);
            char buffer[1024];

            while (1) {
                int bytes_received = recv(newSocket, buffer, sizeof(buffer), 0);
                if (bytes_received <= 0) {
                    printf("Client disconnected from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
                    close(newSocket);
                    exit(0);
                } else {
                    buffer[bytes_received] = '\0'; // Null-terminate the received data
                    printf("Client Sent: %s\n", buffer);

                    unsigned long number = strtoul(buffer, NULL, 10);
                    unsigned long long result = computeFactorial(number);

                    char client_buffer[1024];
                    sprintf(client_buffer, "%llu/0", result); // Send only the factorial result

                    printf("Factorial of %lu is %llu\n", number, result);

                    send(newSocket, client_buffer, strlen(client_buffer), 0);
                    bzero(buffer, sizeof(buffer)); // Clear the buffer for the next message
                }
            }
        }
        close(newSocket);
    }

    close(sockfd);
    return 0;
}

