#include "net_utilities.h"
#include <unistd.h>
#include <errno.h>

int read_all(int fd, void *buf, size_t count) {
    size_t total_read = 0;
    char *ptr = (char *)buf;

    while (total_read < count) {
        ssize_t bytes = read(fd, ptr + total_read, count - total_read);
        if (bytes == 0) {
            return 0; // ה-Socket נסגר על ידי הצד השני
        }
        if (bytes < 0) {
            if (errno == EINTR) {
                continue; // פסיקת Signal - ממשיכים לנסות
            }
            return -1; // שגיאת רשת ממשית
        }
        total_read += (size_t)bytes;
    }
    return 1;
}

int write_all(int fd, const void *buf, size_t count) {
    size_t total_written = 0;
    const char *ptr = (const char *)buf;

    while (total_written < count) {
        ssize_t bytes = write(fd, ptr + total_written, count - total_written);
        if (bytes <= 0) {
            if (bytes < 0 && errno == EINTR) {
                continue;
            }
            return -1; // שגיאת כתיבה
        }
        total_written += (size_t)bytes;
    }
    return 1;
}