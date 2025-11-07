/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <common/output/debug.h>
#include <common/states.h>
#include <fcntl.h>
#include <metrics/common/file.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int filemagic_exists(char *path)
{
    if (access(path, R_OK) != 0) {
        debug("File '%s' can not be accessed", path);
        return_msg(0, strerror(errno));
    }
    return 1;
}

int filemagic_can_read(char *path, int *fd)
{
    if (access(path, R_OK) != 0) {
        debug("File '%s' can not be accessed to read", path);
        return_msg(0, strerror(errno));
    }
    if (fd != NULL) {
        if ((*fd = open(path, O_RDONLY)) < 0) {
            debug("Opening '%s' failed: %s", path, strerror(errno));
            return_msg(0, strerror(errno));
        }
        debug("Opened correctly '%s' (fd%d)", path, *fd);
    }
    return 1;
}

int filemagic_can_write(char *path, int *fd)
{
    if (access(path, W_OK) != 0) {
        return_msg(0, strerror(errno));
    }
    if (fd != NULL) {
        if ((*fd = open(path, O_RDWR)) < 0) {
            debug("Opening '%s' failed: %s", path, strerror(errno));
            return_msg(0, strerror(errno));
        }
        debug("Opened correctly '%s' (fd%d)", path, *fd);
    }
    return 1;
}

static int filemagic_can_msomething(char *format, int index_count, int **fds, int (*can)(char *path, int *fd))
{
    char buffer[PATH_MAX];
    int i;

    for (i = 0; i < index_count; ++i) {
        sprintf(buffer, format, i);
        if (!can(buffer, dont_open)) {
            return 0;
        }
    }
    *fds = calloc((unsigned int) index_count, sizeof(int));
    for (i = 0; i < index_count; ++i) {
        sprintf(buffer, format, i);
        if (!can(buffer, &((*(fds))[i]))) {
            return 0;
        }
    }
    return 1;
}

int filemagic_can_mread(char *format, int index_count, int **fds)
{
    return filemagic_can_msomething(format, index_count, fds, filemagic_can_read);
}

int filemagic_can_mwrite(char *format, int index_count, int **fds)
{
    return filemagic_can_msomething(format, index_count, fds, filemagic_can_write);
}

int filemagic_word_read(int fd, char *word, int reset_position)
{
    int i = 0;
    char c;
    if (reset_position) {
        if (lseek(fd, 0, SEEK_SET) < 0) {
            return_print(0, "while trying to read cpufreq file (%s)", strerror(errno));
        }
    }
    // Does not read ' ' nor '\n'
    while ((read(fd, &c, sizeof(char)) > 0) && !((c == (char) 10) || (c == (char) 32))) {
        word[i] = c;
        i++;
    }
    word[i] = '\0';
    debug("Read in fd%d the word '%s'", fd, word);
    if (i == 0) {
        return_print(0, "While reading '%s' in cpufreq file (%s)", word, strerror(errno));
    }
    return 1;
}

int filemagic_word_write(int fd, char *word, int len, int line_break)
{
    int i, r, p;
    // Adding a line break
    if (line_break) {
        len += 1;
        word[len - 1] = '\n';
        word[len]     = '\0';
    }
    for (i = 0, r = 1, p = len; i < len && r > 0;) {
        r = pwrite(fd, (void *) &word[i], p, i);
        i = i + r;
        p = len - i;
    }
    // Recovering from line break
    if (line_break) {
        word[len - 1] = '\0';
    }
    if (r == -1) {
        return_print(0, "%s when writing '%s' in cpufreq file", strerror(errno), word);
    }
    return 1;
}

int filemagic_word_mwrite(int *fds, int fds_count, char *word, int line_break)
{
    int len = strlen(word);
    int s   = 1;
    int i   = 0;
    debug("Multiple writing a word '%s'", word);
    // Adding a line break
    if (line_break) {
        word[len]     = '\n';
        word[len + 1] = '\0';
        len++;
    }
    for (i = 0; i < fds_count; ++i) {
        debug("Writting in fd%d", fds[i]);
        if (!filemagic_word_write(fds[i], word, len, 0)) {
            s = 0;
        }
    }
    // Recovering from line break
    if (line_break) {
        word[len - 1] = '\0';
    }
    return s;
}

int filemagic_once_read(char *path, char *buffer, int buffer_length)
{
    int fd = -1;
    int i  = 0;
    int r  = 0;
    if ((fd = open(path, O_RDONLY)) < 0) {
        debug("Opening '%s' failed: %s", path, strerror(errno));
        return_msg(0, strerror(errno));
    }
    while (r < buffer_length) {
        if ((r = read(fd, &buffer[r], buffer_length)) <= 0) {
            break;
        }
        buffer_length -= r;
        i += r;
    }
    buffer[i] = '\0';
    debug("Once read: %s", buffer);
    close(fd);
    return 1;
}