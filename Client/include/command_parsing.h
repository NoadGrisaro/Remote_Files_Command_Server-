#ifndef COMMAND_PARSING_H
#define COMMAND_PARSING_H

#define MAX_FILENAME_LEN 128
#define MAX_TEXT_LEN     1024

typedef enum {
    ACTION_READ,
    ACTION_CREATE,
    ACTION_DELETE,
    ACTION_WRITE,
    ACTION_APPEND,
    ACTION_UNKNOWN
} ActionType;

typedef struct {
    ActionType action;
    char file_name[MAX_FILENAME_LEN];
    char text[MAX_TEXT_LEN];
} CommandRequest;

typedef enum {
    PARSE_SUCCESS = 0,
    ERR_EMPTY_INPUT,
    ERR_UNKNOWN_ACTION,
    ERR_MISSING_FILENAME,
    ERR_FILENAME_TOO_LONG,
    ERR_TOO_MANY_ARGUMENTS,
    ERR_MISSING_TEXT,
    ERR_TEXT_TOO_LONG
} ParseStatus;

/**
 * @brief Parses a raw input line into a structured CommandRequest.
 * 
 * Extracts the action keyword, target file name (handling optional quoted tokens),
 * and payload text if required by the operation. Validates token lengths and argument counts.
 * 
 * @param[in]  raw_line  Null-terminated string containing the raw user input.
 * @param[out] out_cmd   Pointer to a CommandRequest structure to receive parsed fields.
 * 
 * @return ParseStatus   PARSE_SUCCESS on valid input, or a specific error code on failure.
 */
ParseStatus parseCommand(const char *raw_line, CommandRequest *out_cmd);

#endif // COMMAND_PARSING_H