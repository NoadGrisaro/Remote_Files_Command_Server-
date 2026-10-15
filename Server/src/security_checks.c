#include "security_checks.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdlib.h>

#define BASE_DIR "/home/noad/Remote_Files_Command_Server/OutPut"

int validateSandboxPath(const char *user_path, char *resolved_out) {
    if (!user_path || !resolved_out || user_path[0] == '\0' || user_path[0] == '/') {
        return 0;
    }

    size_t user_len = strlen(user_path);
    if (user_len >= PATH_MAX) {
        return 0;
    }

    for (size_t i = 0; i < user_len; i++) {
        if (!isprint((unsigned char)user_path[i])) {
            return 0;
        }
    }

    size_t base_len = strlen(BASE_DIR);

    const char *last_slash = strrchr(user_path, '/');
    if (last_slash==NULL) {
        return 0; 
    }

    char user_dir_part[PATH_MAX];
    size_t user_dir_len = (size_t)(last_slash - user_path);
    
    strncpy(user_dir_part, user_path, user_dir_len);
    user_dir_part[user_dir_len] = '\0';

    const char *file_part = last_slash + 1;
    if (file_part[0] == '\0' || strcmp(file_part, ".") == 0 || strcmp(file_part, "..") == 0) {
        return 0;
    }

    char combined_dir[PATH_MAX];
    if (snprintf(combined_dir, sizeof(combined_dir), "%s/%s", BASE_DIR, user_dir_part) >= (int)sizeof(combined_dir)) {
        return 0;
    }

    char resolved_combined_dir[PATH_MAX];
    if (realpath(combined_dir, resolved_combined_dir) == NULL) {
        return 0; 
    }

    if (strncmp(BASE_DIR, resolved_combined_dir, base_len) != 0) {
        return 0;
    }

    if (snprintf(resolved_out, PATH_MAX, "%s/%s", resolved_combined_dir, file_part) >= PATH_MAX) {
        return 0;
    }
    return 1;
}