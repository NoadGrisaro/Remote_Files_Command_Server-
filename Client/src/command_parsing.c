#include <stdio.h>
#include <string.h>
#include <ctype.h>


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


static const char *skipWhitespace(const char *str) {
    while (*str == ' ' || *str == '\t') {
        str++;
    }
    return str;
}

static ActionType ParseAction(const char *word, size_t len) {
    if (len == 4 && strncmp(word, "READ", 4) == 0)   return ACTION_READ;
    if (len == 6 && strncmp(word, "CREATE", 6) == 0) return ACTION_CREATE;
    if (len == 6 && strncmp(word, "DELETE", 6) == 0) return ACTION_DELETE;
    if (len == 5 && strncmp(word, "WRITE", 5) == 0)  return ACTION_WRITE;
    if (len == 6 && strncmp(word, "APPEND", 6) == 0) return ACTION_APPEND;
    return ACTION_UNKNOWN;
}


ParseStatus parse_command(const char *raw_line, CommandRequest *parsed_command) {
    if (!raw_line || !parsed_command) {
        return ERR_EMPTY_INPUT;
    }

    memset(parsed_command, 0, sizeof(CommandRequest));

    const char *p = skipWhitespace(raw_line);
    if (*p == '\0' || *p == '\r' || *p == '\n') {
        return ERR_EMPTY_INPUT;
    }
  
    const char *action_start = p;
    while (*p != '\0' && !isspace((unsigned char)*p)) {
        p++;
    }
    size_t action_len = p - action_start;

    parsed_command->action = CheckIfActionValid(action_start, action_len);
    if (parsed_command->action == ACTION_UNKNOWN) {
        return ERR_UNKNOWN_ACTION;
    }

    p = skipWhitespace(p);
    if (*p == '\0' || *p == '\r' || *p == '\n') {
        return ERR_MISSING_FILENAME;
    }

    const char *file_start = p;
    while (*p != '\0' && !isspace((unsigned char)*p)) {
        p++;
    }
    size_t file_len = p - file_start;

    if (file_len >= MAX_FILENAME_LEN) {
        return ERR_FILENAME_TOO_LONG;
    }

    memcpy(parsed_command->file_name, file_start, file_len);
    parsed_command->file_name[file_len] = '\0';

    int is_group_1 = (parsed_command->action == ACTION_READ ||
                       parsed_command->action == ACTION_CREATE ||
                       parsed_command->action == ACTION_DELETE);

    if (is_group_1) {
        p = skipWhitespace(p);
        if (*p != '\0' && *p != '\r' && *p != '\n') {
            return ERR_TOO_MANY_ARGUMENTS;
        }
        return PARSE_SUCCESS;
    } else {
        p = skipWhitespace(p);
        if (*p == '\0' || *p == '\r' || *p == '\n') {
            return ERR_MISSING_TEXT;
        }

        const char *text_start = p;
        size_t text_len = strlen(text_start);

        while (text_len > 0 && 
              (text_start[text_len - 1] == '\r' || text_start[text_len - 1] == '\n')) {
            text_len--;
        }

        if (text_len >= MAX_TEXT_LEN) {
            return ERR_TEXT_TOO_LONG;
        }

        memcpy(parsed_command->text, text_start, text_len);
        parsed_command->text[text_len] = '\0';

        return PARSE_SUCCESS;
    }
}