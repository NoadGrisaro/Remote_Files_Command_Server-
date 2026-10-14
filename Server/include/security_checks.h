#ifndef SECURITY_CHECKS_H
#define SECURITY_CHECKS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <ctype.h>

#define BASE_DIR "/home/noad/Remote_Files_Command_Server/OutPut"

int validateSandboxPath(const char *user_path, char *resolved_out);

#endif /* SECURITY_CHECKS_H */