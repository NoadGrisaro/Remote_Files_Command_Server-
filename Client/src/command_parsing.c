#include "command_parsing.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

static const char *skipWhitespace(const char *str) {
    while (*str == ' ' || *str == '\t') {
        str++;
    }
    return str;
}

static const char *extractToken(const char *cursor, const char **token_start, size_t *token_len, bool *unclosed_quote) {
    *unclosed_quote = false;
    cursor = skipWhitespace(cursor);

    if (*cursor == '\0' || *cursor == '\r' || *cursor == '\n') {
        *token_start = NULL;
        *token_len = 0;
        return cursor;
    }

    if (*cursor == '"') {
        cursor++;
        *token_start = cursor;

        while (*cursor != '\0' && *cursor != '"' && *cursor != '\r' && *cursor != '\n') {
            cursor++;
        }

        if (*cursor != '"') {
            *unclosed_quote = true; 
            return cursor;
        }

        *token_len = (size_t)(cursor - *token_start);
        cursor++; /* דילוג על המירכאות הסוגרות */
        return cursor;
    }

    *token_start = cursor;
    while (*cursor != '\0' && !isspace((unsigned char)*cursor)) {
        cursor++;
    }
    *token_len = (size_t)(cursor - *token_start);

    return cursor;
}

static ActionType parseAction(const char *word, size_t len) {
    if (len == 4 && strncmp(word, "READ", 4) == 0)   return ACTION_READ;
    if (len == 6 && strncmp(word, "CREATE", 6) == 0) return ACTION_CREATE;
    if (len == 6 && strncmp(word, "DELETE", 6) == 0) return ACTION_DELETE;
    if (len == 5 && strncmp(word, "WRITE", 5) == 0)  return ACTION_WRITE;
    if (len == 6 && strncmp(word, "APPEND", 6) == 0) return ACTION_APPEND;
    return ACTION_UNKNOWN;
}

static bool actionRequiresPayload(ActionType action) {
    return (action == ACTION_WRITE || action == ACTION_APPEND);
}

static ParseStatus extractPayload(const char *cursor, char *dest, size_t max_len) {
    cursor = skipWhitespace(cursor);
    if (*cursor == '\0' || *cursor == '\r' || *cursor == '\n') {
        return ERR_MISSING_TEXT;
    }

    size_t text_len = strlen(cursor);

    while (text_len > 0 && (cursor[text_len - 1] == '\r' || cursor[text_len - 1] == '\n')) {
        text_len--;
    }

    if (text_len >= max_len) {
        return ERR_TEXT_TOO_LONG;
    }

    memcpy(dest, cursor, text_len);
    dest[text_len] = '\0';

    return PARSE_SUCCESS;
}

ParseStatus parseCommand(const char *raw_line, CommandRequest *parsed_command) {
    if (!raw_line || !parsed_command) {
        return ERR_EMPTY_INPUT;
    }

    memset(parsed_command, 0, sizeof(CommandRequest));

    const char *token_start = NULL;
    size_t token_len = 0;
    bool unclosed_quote = false;

    const char *cursor = extractToken(raw_line, &token_start, &token_len, &unclosed_quote);
    if (token_len == 0) {
        return ERR_EMPTY_INPUT;
    }

    parsed_command->action = parseAction(token_start, token_len);
    if (parsed_command->action == ACTION_UNKNOWN) {
        return ERR_UNKNOWN_ACTION;
    }

    cursor = extractToken(cursor, &token_start, &token_len, &unclosed_quote);
    if (unclosed_quote) {
        return ERR_UNKNOWN_ACTION; 
    }

    if (token_len == 0) {
        return ERR_MISSING_FILENAME;
    }

    if (token_len >= MAX_FILENAME_LEN) {
        return ERR_FILENAME_TOO_LONG;
    }

    memcpy(parsed_command->file_name, token_start, token_len);
    parsed_command->file_name[token_len] = '\0';

    if (actionRequiresPayload(parsed_command->action)) {
        return extractPayload(cursor, parsed_command->text, MAX_TEXT_LEN);
    }

    cursor = skipWhitespace(cursor);
    if (*cursor != '\0' && *cursor != '\r' && *cursor != '\n') {
        return ERR_TOO_MANY_ARGUMENTS;
    }

    return PARSE_SUCCESS;
}