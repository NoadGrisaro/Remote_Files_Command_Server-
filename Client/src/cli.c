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
#define SERVER_PORT 8000
#define BUFFER_SIZE 2048

static int buildProtocolMessage(const CommandRequest *cmd, ProtocolMessage *msg) {
    memset(msg, 0, sizeof(ProtocolMessage));
    msg->magic = htonl(PROTOCOL_MAGIC);

    switch (cmd->action) {
        case ACTION_READ:   msg->opcode = CMD_READ;   break;
        case ACTION_CREATE: msg->opcode = CMD_CREATE; break;
        case ACTION_DELETE: msg->opcode = CMD_DELETE; break;
        case ACTION_WRITE:  msg->opcode = CMD_WRITE;  break;
        case ACTION_APPEND: msg->opcode = CMD_APPEND; break;
        default:            return -1;
    }

    strncpy(msg->filename, cmd->file_name, sizeof(msg->filename) - 1);

    uint32_t payload_len = strlen(cmd->text);
    msg->payload_len = htonl(payload_len);
    if (payload_len > 0) {
        memcpy(msg->payload, cmd->text, payload_len);
    }

    return 0;
}

static int readUserInput(char *buffer, size_t size) {
    printf("Enter a valid command:\nREAD/WRITE/APPEND/CREATE/DELETE + FILE NAME + TEXT (only for WRITE/APPEND)\n to end write 'exit' \n>");
    if (!fgets(buffer, size, stdin)) {
        return -1;
    }

    buffer[strcspn(buffer, "\r\n")] = '\0';
    return 0;
}

static int connectToServer(const char *ip, uint16_t port) {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Socket creation error");
        return -1;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port)
    };

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        close(client_fd);
        return -1;
    }

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        close(client_fd);
        return -1;
    }

    return client_fd;
}

static int sendAndReceive(int client_fd, const ProtocolMessage *req, ProtocolMessage *res) {
    if (writeAll(client_fd, req, sizeof(*req)) <= 0) {
        fprintf(stderr, "Failed to send request\n");
        return -1;
    }

    if (readAll(client_fd, res, sizeof(*res)) <= 0) {
        fprintf(stderr, "Failed to receive response\n");
        return -1;
    }

    if (ntohl(res->magic) != PROTOCOL_MAGIC) {
        fprintf(stderr, "Invalid magic from server\n");
        return -1;
    }

    return 0;
}

static void printResponse(const ProtocolMessage *res) {
    printf("Status: %d\n", res->opcode);
    if (ntohl(res->payload_len) > 0) {
        printf("Response: %s\n", res->payload);
    }
}

static int runSingleCommand(int client_fd) {
    char input_line[BUFFER_SIZE];
    if (readUserInput(input_line, sizeof(input_line)) != 0) {
        return -1;
    }

    if (strcmp(input_line, "exit") == 0 || strlen(input_line) == 0) {
        return 0; 
    }

    CommandRequest parsed_cmd;
    ParseStatus parse_res = parseCommand(input_line, &parsed_cmd);
    if (parse_res != PARSE_SUCCESS) {
        fprintf(stderr, "Command parsing failed with error code: %d\n", parse_res);
        return 1; 
    }

    ProtocolMessage request;
    if (buildProtocolMessage(&parsed_cmd, &request) != 0) {
        fprintf(stderr, "Invalid command action\n");
        return 1;
    }

    ProtocolMessage response;
    if (sendAndReceive(client_fd, &request, &response) != 0) {
        return -1; 
    }

    printResponse(&response);
    return 1; 
}

static void runClientLoop(int client_fd) {
    while (1) {
        int status = runSingleCommand(client_fd);
        if (status <= 0) {
            break;
        }
    }
}

int main(void) {
    int client_fd = connectToServer(SERVER_IP, SERVER_PORT);
    if (client_fd > 0) {
        runClientLoop(client_fd);
    }

    close(client_fd);
    return 0;
}