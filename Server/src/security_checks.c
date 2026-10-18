#include "security_checks.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>

#define BASE_DIR "/home/noad/Remote_Files_Command_Server/OutPut"

static int validateInputString(const char *user_path, const char *resolved_out) {
    if (!user_path || !resolved_out || user_path[0] == '\0' || user_path[0] == '/') {
        return 0;
    }

    size_t user_len = strlen(user_path);
    if (user_len >= PATH_MAX) {
        return 0;
    }

    for (size_t i = 0; i < user_len; i++) {
        if (!isprint((unsigned char)user_path[i])) {//מוודא שכל התווים קריאים ואין תווי בקרה
            return 0;
        }
    }

    return 1;
}

static int extractPathParts(const char *user_path, char *user_dir_part, const char **file_part) {
    const char *last_slash = strrchr(user_path, '/');
    if (last_slash == NULL) {
        return 0;
    }

    size_t user_dir_len = (size_t)(last_slash - user_path);
    strncpy(user_dir_part, user_path, user_dir_len);
    user_dir_part[user_dir_len] = '\0';

    *file_part = last_slash + 1;
    if ((*file_part)[0] == '\0' || strcmp(*file_part, ".") == 0 ||
    strcmp(*file_part, "..") == 0) {//מוודא ששם הקובץ תקין
        return 0;
    }

    return 1;
}

static int resolveAndCheckDir(const char *user_dir_part, char *resolved_combined_dir) {
    char combined_dir[PATH_MAX];
    if (snprintf(combined_dir, sizeof(combined_dir), "%s/%s", BASE_DIR, user_dir_part) >= 
    (int)sizeof(combined_dir)) {// check if base dir+user dir will overload
        return 0;
    }

    if (realpath(combined_dir, resolved_combined_dir) == NULL) {
        return 0;
    }

    size_t base_len = strlen(BASE_DIR);
    if (strncmp(BASE_DIR, resolved_combined_dir, base_len) != 0) {
        return 0;
    }

    return 1;
}

static int buildFinalPath(const char *resolved_dir, const char *file_part, char *resolved_out) {
    if (snprintf(resolved_out, PATH_MAX, "%s/%s", resolved_dir, file_part) 
    >= PATH_MAX) {// check if resolved dir+file will not overload
        return 0;
    }
    return 1;
}

int validateSandboxPath(const char *user_path, char *resolved_out) {
    if (!validateInputString(user_path, resolved_out)) {
        return 0;
    }

    char user_dir_part[PATH_MAX];
    const char *file_part = NULL;
    if (!extractPathParts(user_path, user_dir_part, &file_part)) {
        return 0;
    }

    char resolved_combined_dir[PATH_MAX];
    if (!resolveAndCheckDir(user_dir_part, resolved_combined_dir)) {
        return 0;
    }

    return buildFinalPath(resolved_combined_dir, file_part, resolved_out);
}