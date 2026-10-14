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

static void buildProtocolMessage(const CommandRequest *cmd, ProtocolMessage *msg) {
    memset(msg, 0, sizeof(ProtocolMessage));
    msg->magic = htonl(PROTOCOL_MAGIC);
    
    switch (cmd->action) {
        case ACTION_READ:   msg->opcode = CMD_READ;   break;
        case ACTION_CREATE: msg->opcode = CMD_CREATE; break;
        case ACTION_DELETE: msg->opcode = CMD_DELETE; break;
        case ACTION_WRITE:  msg->opcode = CMD_WRITE;  break;
        case ACTION_APPEND: msg->opcode = CMD_APPEND; break;
        default:            msg->opcode = 0;          break;
    }

    strncpy(msg->filename, cmd->file_name, sizeof(msg->filename) - 1);
    
    uint32_t payload_len = strlen(cmd->text);
    msg->payload_len = htonl(payload_len);
    if (payload_len > 0) {
        memcpy(msg->payload, cmd->text, payload_len);
    }
}

int main(void) {
    char client_input[BUFFER_SIZE];
    CommandRequest parsed_command;
    ProtocolMessage protocol_request;
    ProtocolMessage protocol_response;

    printf("Enter a valid command: \n READ/WRITE/APPEND/CREATE/DELETE + FILE NAME + TEXT (only for WRITE/APPEND)\n ");
    if (!fgets(client_input, sizeof(client_input), stdin)) {
        return -1;
    }

    ParseStatus parse_res = parse_command(client_input, &parsed_command);
    if (parse_res != PARSE_SUCCESS) {
        fprintf(stderr, "Command parsing failed with error code: %d\n", parse_res);
        return -1;
    }

    buildProtocolMessage(&parsed_command, &protocol_request);

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Socket creation error");
        return -1;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT)
    };

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(client_fd);
        return -1;
    }

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        close(client_fd);
        return -1;
    }

    if (write_all(client_fd, &protocol_request, sizeof(protocol_request)) <= 0) {
        fprintf(stderr, "Failed to send request\n");
        close(client_fd);
        return -1;
    }

    if (read_all(client_fd, &protocol_response, sizeof(protocol_response)) <= 0) {
        fprintf(stderr, "Failed to receive response\n");
        close(client_fd);
        return -1;
    }

    if (ntohl(protocol_response.magic) != PROTOCOL_MAGIC) {
        fprintf(stderr, "Invalid magic from server\n");
    } else {
        printf("Status: %d\n", protocol_response.opcode);
        if (ntohl(protocol_response.payload_len) > 0) {
            printf("Response: %s\n", protocol_response.payload);
        }
    }

    close(client_fd);
    return 0;
}