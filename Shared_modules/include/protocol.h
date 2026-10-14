#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define PROTOCOL_MAGIC 0x53465358 // "SFSX"
#define MAX_FILENAME_LEN 128
#define MAX_TEXT_LEN     1024

typedef enum {
    CMD_READ = 1,
    CMD_CREATE = 2,
    CMD_DELETE = 3,
    CMD_WRITE  = 4,
    CMD_APPEND = 5
} CommandOpcode;

typedef enum {
    STATUS_OK = 0,
    ERR_BAD_REQUEST = 1,
    ERR_SANDBOX_VIOLATION = 2,
    ERR_FILE_BUSY = 3,
    ERR_FILE_NOT_FOUND = 4,
    ERR_FILE_EXISTS = 5,
    ERR_IO_FAILURE = 6,
    ERR_INTERNAL_SERVER = 7
} StatusCode;

typedef struct {
    uint32_t magic;
    uint8_t  opcode;
    char filename[MAX_FILENAME_LEN];
    uint32_t payload_len;
    char payload[MAX_TEXT_LEN];
} ProtocolMessage;

#endif // PROTOCOL_H