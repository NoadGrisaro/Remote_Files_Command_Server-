#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <stddef.h>
#include <stdint.h>

int read_all(int fd, char *buf, size_t count);

int write_all(int fd, const char*buf, size_t count);

#endif // NET_UTILS_H