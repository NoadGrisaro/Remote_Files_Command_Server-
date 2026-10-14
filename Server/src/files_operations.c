#include "files_operations.h"
#include <string.h>
#include <limits.h>
#include <pthread.h>

#define MAX_CONCURRENT_WRITERS 32

typedef struct {
    char paths[MAX_CONCURRENT_WRITERS][PATH_MAX];
    size_t count;
    pthread_mutex_t lock;
} WriterTable;

static WriterTable writer_table = {
    .count = 0,
    .lock = PTHREAD_MUTEX_INITIALIZER
};

static int findIndexUnlocked(const char *file_path) {
    for (size_t i = 0; i < writer_table.count; i++) {
        if (strcmp(writer_table.paths[i], file_path) == 0) {
            return (int)i;
        }
    }
    return -1;
}

int addNewWriter(const char *path) {
    if (!path) return 0;

    pthread_mutex_lock(&writer_table.lock);

    if (writer_table.count >= MAX_CONCURRENT_WRITERS || find_index_unlocked(path) != -1) {
        pthread_mutex_unlock(&writer_table.lock);
        return 0;
    }

    strncpy(writer_table.paths[writer_table.count], path, PATH_MAX - 1);
    writer_table.paths[writer_table.count][PATH_MAX - 1] = '\0';
    writer_table.count++;

    pthread_mutex_unlock(&writer_table.lock);
    return 1;
}

int releaseWriter(const char *path) {
    if (!path) return 0;

    pthread_mutex_lock(&writer_table.lock);

    int idx = find_index_unlocked(path);
    if (idx == -1) {
        pthread_mutex_unlock(&writer_table.lock);
        return 0;
    }

    size_t last_idx = writer_table.count - 1;
    if ((size_t)idx != last_idx) {
        strncpy(writer_table.paths[idx], writer_table.paths[last_idx], PATH_MAX);
    }
    writer_table.count--;

    pthread_mutex_unlock(&writer_table.lock);
    return 1;
}