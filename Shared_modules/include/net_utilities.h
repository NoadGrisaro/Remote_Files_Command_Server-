#ifndef NET_UTILITIES_H
#define NET_UTILITIES_H

#include <stddef.h>

int read_all(int fd, void *buf, size_t count);
int write_all(int fd, const void *buf, size_t count);

#endif // NET_UTILITIES_H