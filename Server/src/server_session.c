#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>


#include "server_session.h"
#include "security_checks.h"
#include "files_operations.h"
#include "protocol.h"
#include "net_utilities.h"

void handleClientSession(int client_fd, const ProtocolMessage *protocol_req) {
    ProtocolMessage protocol_res;
    memset(&protocol_res, 0, sizeof(protocol_req));
    protocol_res.magic = htonl(PROTOCOL_MAGIC);

    char safe_filename[MAX_FILENAME_LEN];
    strncpy(safe_filename, protocol_req->filename, MAX_FILENAME_LEN - 1);
    safe_filename[MAX_FILENAME_LEN - 1] = '\0';

    char resolved_path[PATH_MAX];

    if (!validateSandboxPath(safe_filename, resolved_path)) {
        protocol_res.opcode = ERR_SANDBOX_VIOLATION;
        write_all(client_fd, &protocol_res, sizeof(protocol_res));
        return;
    }

    if (protocol_req->opcode == CMD_CREATE || protocol_req->opcode == CMD_WRITE || 
        protocol_req->opcode == CMD_APPEND || protocol_req->opcode == CMD_DELETE) {

        if (!addNewWriter(resolved_path)) {
            protocol_res.opcode = ERR_FILE_BUSY;
            write_all(client_fd, &protocol_res, sizeof(protocol_res));
            return;
        }

        uint32_t payload_len = ntohl(protocol_req->payload_len);

        switch (protocol_req->opcode) {
            case CMD_CREATE: {
                int fd = open(resolved_path, O_CREAT | O_EXCL | O_WRONLY, 0644);
                if (fd < 0) {
                    protocol_res.opcode = (errno == EEXIST) ? ERR_FILE_EXISTS : ERR_IO_FAILURE;
                } else {
                    close(fd);
                    protocol_res.opcode = STATUS_OK;
                }
                break;
            }

            case CMD_WRITE: {
                int fd = open(resolved_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
                if (fd < 0) {
                    protocol_res.opcode = ERR_IO_FAILURE;
                } else {
                    if (payload_len > 0 && write_all(fd, protocol_req->payload, payload_len) <= 0) {
                        protocol_res.opcode = ERR_IO_FAILURE;
                    } else {
                        protocol_res.opcode = STATUS_OK;
                    }
                    close(fd);
                }
                break;
            }

            case CMD_APPEND: {
                int fd = open(resolved_path, O_WRONLY | O_APPEND);
                if (fd < 0) {
                    protocol_res.opcode = (errno == ENOENT) ? ERR_FILE_NOT_FOUND : ERR_IO_FAILURE;
                } else {
                    if (payload_len > 0 && write_all(fd, protocol_req->payload, payload_len) <= 0) {
                        protocol_res.opcode = ERR_IO_FAILURE;
                    } else {
                        protocol_res.opcode = STATUS_OK;
                    }
                    close(fd);
                }
                break;
            }

            case CMD_DELETE: {
                if (unlink(resolved_path) < 0) {
                    protocol_res.opcode = (errno == ENOENT) ? ERR_FILE_NOT_FOUND : ERR_IO_FAILURE;
                } else {
                    protocol_res.opcode = STATUS_OK;
                }
                break;
            }

            default:
                protocol_res.opcode = ERR_BAD_REQUEST;
                break;
        }

        releaseWriter(resolved_path);

    } else if (protocol_req->opcode == CMD_READ) {

        int fd = open(resolved_path, O_RDONLY);
        if (fd < 0) {
            protocol_res.opcode = (errno == ENOENT) ? ERR_FILE_NOT_FOUND : ERR_IO_FAILURE;
        } else {
            ssize_t bytes_read = read(fd, protocol_res.payload, MAX_TEXT_LEN - 1);
            if (bytes_read < 0) {
                protocol_res.opcode = ERR_IO_FAILURE;
            } else {
                protocol_res.opcode = STATUS_OK;
                protocol_res.payload[bytes_read] = '\0';
                protocol_res.payload_len = htonl((uint32_t)bytes_read);
            }
            close(fd);
        }
    } else {
        protocol_res.opcode = ERR_BAD_REQUEST;
    }

    write_all(client_fd, &protocol_res, sizeof(protocol_res));
}