#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "command_parsing.h"
#include "protocol.h"
#include "net_utilities.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9000
#define BUFFER_SIZE 2048

int main(void) {
    char client_input[BUFFER_SIZE];
    CommandRequest parsed_command;
    ProtocolMessage protocol_request;
    ProtocolMessage protocol_response;

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);


    printf("Enter a valid command: ");
    if (!fgets(client_input, sizeof(client_input), stdin)) {
        perror("Error reading input\n");
        return -1;
    }

    ParseStatus parse_res = parse_command(client_input, &parsed_command);
    if (parse_res != PARSE_SUCCESS) {
        perror("Command parsing failed with error code: %d\n", parse_res);
        return -1;
    }

    buildProtocol(&parsed_command, &protocol_request);

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Socket creation error");
        return -1;
    }

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported\n");
        close(client_fd);
        return -1;
    }

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        close(client_fd);
        return -1;
    }

    if (TransferReqToServer(client_fd, &protocol_request) < 0) {
        perror("Failed to send request\n");
        close(client_fd);
        return -1;
    }

    ssize_t bytes_received = recv(client_fd, &protocol_response, sizeof(protocol_response), 0);
    if (bytes_received <= 0) {
        perror("Failed to receive response from server\n");
        close(client_fd);
        return -1;
    } else {
        printf("Response received successfully\n");
    }

    close(client_fd);
    return 0;
}