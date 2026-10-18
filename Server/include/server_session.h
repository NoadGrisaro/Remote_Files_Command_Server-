#ifndef SERVER_SESSION_H
#define SERVER_SESSION_H

#include "protocol.h"

/**
 * @brief Processes a single client request and transmits the response back over TCP.
 * 
 * Validates the requested target file against the sandbox constraints, dispatches
 * the operation to the appropriate file handler (read, write, append, create, delete)
 * under concurrency locks, and sends the resulting ProtocolMessage to the client.
 * 
 * @param[in] client_fd Open socket file descriptor for communication with the client.
 * @param[in] req       Pointer to the incoming validated ProtocolMessage request.
 */
void handleClientSession(int client_fd, const ProtocolMessage *req);

#endif // SERVER_SESSION_H