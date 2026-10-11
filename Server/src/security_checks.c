#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <ctype.h>

#define BASE_DIR "/home/noad/Remote_Files_Command_Server/OutPut"

static int isValidFileName(const char *filename) {
    if (!filename || filename[0] == '\0') {
        return 0;
    }

    // חסימת שמות שמורים או נסתרים
    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0 || filename[0] == '.') {
        return 0;
    }

    size_t len = strlen(filename);
    if (len > 128) {
        return 0;
    }

    for (size_t i = 0; i < len; i++) {
        char c = filename[i];
        if (!isalnum((unsigned char)c) && c != '.' && c != '_' && c != '-') {
            return 0; // חוסם רווחים, לוכסנים, ותווי מעטפת
        }
    }

    return 1;
}


int resolveSafeNewFilePath(const char *user_input, char *out_full_path, size_t out_size) {
    if (!user_input || user_input[0] == '\0' || !out_full_path) {
        return 0;
    }

    // חסימה מוקדמת של נתיבים מוחלטים
    if (user_input[0] == '/' || user_input[0] == '\\') {
        return 0;
    }

    char path_copy1[PATH_MAX];
    char path_copy2[PATH_MAX];
    strncpy(path_copy1, user_input, sizeof(path_copy1) - 1);
    strncpy(path_copy2, user_input, sizeof(path_copy2) - 1);
    path_copy1[sizeof(path_copy1) - 1] = '\0';
    path_copy2[sizeof(path_copy2) - 1] = '\0';

    // פירוק לנתיב תיקייה ושם קובץ
    char *dir_part = dirname(path_copy1);
    char *file_part = basename(path_copy2);

    // 1. בדיקת חוקיות שם הקובץ הבודד
    if (!is_valid_filename(file_part)) {
        return 0;
    }

    // 2. פתרון תיקיית הבסיס המורשית
    char base_resolved[PATH_MAX];
    if (realpath(BASE_DIR, base_resolved) == NULL) {
        return 0; // תיקיית הבסיס אינה קיימת בדיסק
    }

    // 3. שרשור תיקיית היעד לתיקיית הבסיס
    char target_dir_combined[PATH_MAX];
    snprintf(target_dir_combined, sizeof(target_dir_combined), "%s/%s", base_resolved, dir_part);

    // 4. פתרון התיקייה הסופית (היא חייבת להיות קיימת בדיסק!)
    char target_dir_resolved[PATH_MAX];
    if (realpath(target_dir_combined, target_dir_resolved) == NULL) {
        return 0; // התיקייה שבה רוצים לשים את הקובץ אינה קיימת
    }

    // 5. בדיקת הכלה: האם תיקיית היעד מוכלת בתוך BASE_DIR?
    size_t base_len = strlen(base_resolved);
    if (strncmp(target_dir_resolved, base_resolved, base_len) != 0) {
        return 0; // ניסיון בריחה של התיקייה החוצה!
    }

    // בדיקת גבול תיקייה (Border check)
    if (target_dir_resolved[base_len] != '/' && target_dir_resolved[base_len] != '\0') {
        return 0;
    }

    // 6. הרכבת הנתיב המלא המאומת
    snprintf(out_full_path, out_size, "%s/%s", target_dir_resolved, file_part);
    return 1;
}


