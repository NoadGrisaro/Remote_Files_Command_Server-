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


ParseStatus parse_command(const char *raw_line, CommandRequest *out_cmd);

#endif // COMMAND_PARSING_H