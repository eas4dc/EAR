/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// clang-format off
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <metrics/common/script.h>

#define SCRIPT_BUFFER_SIZE 1024

extern char **environ;

int script_is_executable(const char *path)
{
    struct stat info = {0};

    // stat sigue los enlaces simbólicos; solo aceptamos archivos regulares ejecutables.
    return
        (path != NULL) &&
        (path[0] != '\0') &&
        (stat(path, &info) == 0) &&
        (S_ISREG(info.st_mode)) &&
        (access(path, X_OK) == 0);
}

int script_exec(const char *path, int wait, int quiet, const char *format, ...)
{
    posix_spawn_file_actions_t action = {0};
    char      buffer[SCRIPT_BUFFER_SIZE];
    char     *argv  [16];
    size_t    argc   = 0;
    pid_t     pid    = 0;
    char     *cursor = NULL;
    int       length = 0;
    int       result = 0;
    int       status = 0;
    va_list   args;

    if (!action.__allocated) {
        posix_spawn_file_actions_addopen(
            &action, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
    }
    if (path == NULL || path[0] == '\0' || format == NULL) {
        errno = EINVAL;
        return 0;
    }
    // To know the required length.
    va_start(args, format);
    length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    // Controlling overflows.
    if (length < 0) {
        return 0;
    }
    if ((size_t) length >= sizeof(buffer)) {
        errno = E2BIG;
        return 0;
    }
    // argv[0] is the executable and the following elements are arguments.
    argv[0] = (char *) path;
    cursor = buffer;
    while (*cursor != '\0') {
        // Each word is an argument.
        while (isspace((unsigned char) *cursor)) {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }
        if (argc == (1024 - 2)) {
            errno = E2BIG;
            return 0;
        }
        argv[++argc] = cursor;
        while (*cursor != '\0' && !isspace((unsigned char) *cursor)) {
            ++cursor;
        }
        if (*cursor != '\0') {
            // Next argument.
            *cursor++ = '\0';
        }
    }
    argv[argc + 1] = NULL;
    // Launching the process
    if ((result = posix_spawnp(&pid, path, quiet? &action: NULL, NULL, argv, environ)) != 0) {
        // posix_spawnp returns the error code, instead putting it in errno.
        errno = result;
        return 0;
    }
    if (!wait) {
        return 1;
    }
    // Waiting for child process.
    do {
        result = waitpid(pid, &status, 0);
    } while (result == -1 && errno == EINTR);
    //
    if (result == -1) {
        return 0;
    }
    if (WIFEXITED(status)) {
        return (WEXITSTATUS(status) == 0);
    }
    if (WIFSIGNALED(status)) {
        return 0;
    }
    errno = ECHILD;
    return 0;
}

#ifdef TEST
int main(int argc, char *argv[])
{
    int result = 0;
    errno = 0;

    result = script_exec(argv[1], 0, "%d %d", 69, 1337);
    printf("script_exec: %d (%d: %s)\n", result, errno, strerror(errno));
    return 0;
}
#endif