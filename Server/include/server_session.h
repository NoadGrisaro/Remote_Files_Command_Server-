#ifndef SERVER_SESSION_H
#define SERVER_SESSION_H

#include "protocol.h"

void handleClientSession(int client_fd, const ProtocolMessage *req);

#endif // SERVER_SESSION_H