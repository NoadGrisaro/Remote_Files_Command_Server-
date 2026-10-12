#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "protocol.h"
#include "net_utilities.h"
#include "server_session.h"

#define SERVER_PORT 8000
#define BACKLOG 10

int main() {
    int server_fd;
    struct sockaddr_in server_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Server socket creation error");
        return -1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("Listen failed");
        close(server_fd);
        return -1;
    }

    printf("Server listening on port %d...\n", SERVER_PORT);

  
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Accept failed");
            continue; 
        }

        printf("New client connected from %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        ProtocolMessage protocol_request;
        ssize_t bytes_received = recv(client_fd, &protocol_request, sizeof(protocol_request), 0);

        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("Client disconnected before sending data\n");
            } else {
                perror("Recv failed");
            }
            close(client_fd);
            continue;
        }

        printf("Request received successfully (%zd bytes)\n", bytes_received);

        handleClientSession(client_fd, &protocol_request);



        if (TransferResToClient(client_fd, &protocol_request) < 0) {
            perror("Failed to send response\n");
            close(client_fd);
            return -1;
        }



        close(client_fd);
    }

    close(server_fd);
    return 0;
}