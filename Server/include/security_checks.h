#ifndef SECURITY_CHECKS_H
#define SECURITY_CHECKS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>
#include <ctype.h>

#define BASE_DIR "/home/noad/Remote_Files_Command_Server/OutPut"

/**
 * @brief Validates and resolves a user-provided path within the designated sandbox directory.
 * 
 * Performs input sanitization, prevents path traversal attacks (e.g., via "../"),
 * verifies that the target parent directory exists and resides strictly under BASE_DIR,
 * and constructs the absolute resolved canonical path.
 * 
 * @param[in]  user_path     The relative file path submitted by the user.
 * @param[out] resolved_out  Output buffer of size at least PATH_MAX to receive the canonical path.
 * 
 * @return 1 if the path is safe and successfully resolved within BASE_DIR, 0 otherwise.
 */
int validateSandboxPath(const char *user_path, char *resolved_out);

#endif /* SECURITY_CHECKS_H */