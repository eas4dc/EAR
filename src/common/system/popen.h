/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_POPEN_H
#define METRICS_COMMON_POPEN_H

#include <common/states.h>

typedef struct popen_s {
    char table[512][16][128];
    int rows_escape;
    int rows_index;
    int rows_count;
    int cols_count;
    int one_shot;
    int opened;
    FILE *file;
    int eof;
    int fd;
    pid_t pid;
} popen_t;

// Tests if an executable (binary or script) exists, before call popen_open.
state_t popen_test(char *exec);

// Opens a process given a command.
//     @param escape_lines: avoids reading the first N lines. It can be useful
//         to escape the typical header returned by the command.
//     @param one_shot: used when the command process prints something and
//         immediately closes and is not kept in the background writing output
//         intermittently. Under the hood blocks ead functions ensuring you have
//         something to read and compress the data allowing very large outputs.
state_t popen_open(char *command, int escape_lines, int one_shot, popen_t *p);

void popen_close(popen_t *p);

// Reads a single line of the command output. It is a variadic function which
// converts the white spaced separated line of string format into a set of
// variables of different type.
//     @param fmt: it is the format and admits the following letters:
//         - a: avoid (pass to next read argument)
//         - s: returns a string (char **, a pointer to an empty char pointer)
//         - S: returns a string vector (char **, a pointer to an empty char pointer vector)
//         - i: returns an integer (int *, a pointer to int)
//         - I: returns an integer vector (int *, a pointer to int vector)
//         - d: returns a double (double *, a pointer to double)
//         - D: returns a double vector (double *, a pointer to double vector)
//         Take into the account that vector letters are used as last letters
//         in format because the function doesn't know in advance how many
//         elements the caller wants to read. You can see the example in the
//         test function of the source file. For more information look at TEST1
//         and TEST2 tests in the C source file.
//     @returns if there are lines pending to read.
#define popen_read(p, fmt, ...) popen_read2(p, ' ', fmt, __VA_ARGS__)

// The same than popen_read() macro but using a separator instead white space.
int popen_read2(popen_t *p, char separator, const char *fmt, ...);

// Counts the number of current lines kept in the buffer from last reading.
uint popen_count_read(popen_t *p);

// Count the number of lines pending.
uint popen_count_pending(popen_t *p);

#endif // METRICS_COMMON_POPEN_H
