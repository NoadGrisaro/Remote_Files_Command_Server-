#ifndef NET_UTILITIES_H
#define NET_UTILITIES_H

#include <stddef.h>

/**
 * @brief Reads an exact number of bytes from a descriptor, handling partial reads and interrupts.
 * 
 * Loops continuously until all requested bytes are read, the peer closes the connection,
 * or an unrecoverable socket/file error occurs. Automatically retries if interrupted by a signal (EINTR).
 * 
 * @param[in]  fd    Open file or socket descriptor to read from.
 * @param[out] buf   Destination buffer to store the incoming bytes.
 * @param[in]  count Exact number of bytes required to read.
 * 
 * @return 1 if all count bytes were read successfully, 0 on peer disconnect (EOF), or -1 on network/read error.
 */
int readAll(int fd, void *buf, size_t count);

/**
 * @brief Writes an exact number of bytes to a descriptor, handling partial writes and interrupts.
 * 
 * Loops continuously until all specified bytes are transmitted or an error occurs.
 * Automatically retries if interrupted by a signal (EINTR).
 * 
 * @param[in] fd    Open file or socket descriptor to write to.
 * @param[in] buf   Source buffer containing the data to transmit.
 * @param[in] count Exact number of bytes to write.
 * 
 * @return 1 if all count bytes were written successfully, or -1 on network/write error.
 */
int writeAll(int fd, const void *buf, size_t count);

#endif // NET_UTILITIES_H