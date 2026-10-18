#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdbool.h>

#include "server_session.h"
#include "security_checks.h"
#include "files_operations.h"
#include "protocol.h"
#include "net_utilities.h"

static uint8_t doFileCreate(const char *path) {
    int fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd < 0) {
        return (errno == EEXIST) ? ERR_FILE_EXISTS : ERR_IO_FAILURE;
    }
    close(fd);
    return STATUS_OK;
}

static uint8_t doFileWrite(const char *path, const char *payload, uint32_t len) {
    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        return ERR_IO_FAILURE;
    }

    if (len > 0 && writeAll(fd, payload, len) <= 0) {
        close(fd);
        return ERR_IO_FAILURE;
    }

    close(fd);
    return STATUS_OK;
}

static uint8_t doFileAppend(const char *path, const char *payload, uint32_t len) {
    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd < 0) {
        return (errno == ENOENT) ? ERR_FILE_NOT_FOUND : ERR_IO_FAILURE;
    }

    if (len > 0 && writeAll(fd, payload, len) <= 0) {
        close(fd);
        return ERR_IO_FAILURE;
    }

    close(fd);
    return STATUS_OK;
}

static uint8_t doFileDelete(const char *path) {
    if (unlink(path) < 0) {
        return (errno == ENOENT) ? ERR_FILE_NOT_FOUND : ERR_IO_FAILURE;
    }
    return STATUS_OK;
}

static uint8_t doFileRead(const char *path, ProtocolMessage *res) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return (errno == ENOENT) ? ERR_FILE_NOT_FOUND : ERR_IO_FAILURE;
    }

    ssize_t bytes_read = read(fd, res->payload, MAX_TEXT_LEN - 1);
    close(fd);

    if (bytes_read < 0) {
        return ERR_IO_FAILURE;
    }

    res->payload[bytes_read] = '\0';
    res->payload_len = htonl((uint32_t)bytes_read);
    return STATUS_OK;
}

static uint8_t executeWriteOperation(const char *path, const ProtocolMessage *req) {
    if (!addNewWriter(path)) {
        return ERR_FILE_BUSY;
    }

    uint32_t payload_len = ntohl(req->payload_len);
    uint8_t status;

    switch (req->opcode) {
        case CMD_CREATE:
            status = doFileCreate(path);
            break;
        case CMD_WRITE:
            status = doFileWrite(path, req->payload, payload_len);
            break;
        case CMD_APPEND:
            status = doFileAppend(path, req->payload, payload_len);
            break;
        case CMD_DELETE:
            status = doFileDelete(path);
            break;
        default:
            status = ERR_BAD_REQUEST;
            break;
    }

    releaseWriter(path);
    return status;
}

static bool isWriteAction(uint8_t opcode) {
    return (opcode == CMD_CREATE || opcode == CMD_WRITE || 
            opcode == CMD_APPEND || opcode == CMD_DELETE);
}

void handleClientSession(int client_fd, const ProtocolMessage *protocol_req) {
    ProtocolMessage protocol_res;
    memset(&protocol_res, 0, sizeof(protocol_res));
    protocol_res.magic = htonl(PROTOCOL_MAGIC);

    char safe_filename[MAX_FILENAME_LEN];
    strncpy(safe_filename, protocol_req->filename, MAX_FILENAME_LEN - 1);
    safe_filename[MAX_FILENAME_LEN - 1] = '\0';

    char resolved_path[PATH_MAX];

    if (!validateSandboxPath(safe_filename, resolved_path)) {
        protocol_res.opcode = ERR_SANDBOX_VIOLATION;
        writeAll(client_fd, &protocol_res, sizeof(protocol_res));
        return;
    }

    if (isWriteAction(protocol_req->opcode)) {
        protocol_res.opcode = executeWriteOperation(resolved_path, protocol_req);
    } else if (protocol_req->opcode == CMD_READ) {
        protocol_res.opcode = doFileRead(resolved_path, &protocol_res);
    } else {
        protocol_res.opcode = ERR_BAD_REQUEST;
    }

    writeAll(client_fd, &protocol_res, sizeof(protocol_res));
}