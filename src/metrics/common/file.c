/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* EAR is an open source software, and it is licensed under both the BSD-3 license
* and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
* and COPYING.EPL files.
*/

//#define SHOW_DEBUGS 1

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/states.h>
#include <common/output/debug.h>
#include <metrics/common/file.h>

int filemagic_can_read(char *path, int *fd)
{
    if (access(path, R_OK) != 0) {
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
    *fds = calloc(index_count, sizeof(int));
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
        if(lseek(fd, 0, SEEK_SET) < 0) {
            return_print(0, "while trying to read cpufreq file (%s)", strerror(errno));
        }
    }
    // Does not read ' ' nor '\n'
    while((read(fd, &c, sizeof(char)) > 0) && !((c == (char) 10) || (c == (char) 32))) {
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

int filemagic_word_write(int fd, char *word, int s, int line_break)
{
    int i, r, p;
    // Adding a line break
    if (line_break) {
        s += 1;
        word[s-1] = '\n';
        word[s] = '\0';
    }
    for (i = 0, r = 1, p = s; i < s && r > 0;) {
        r = pwrite(fd, (void *) &word[i], p, i);
        i = i + r;
        p = s - i;
    }
    if (r == -1) {
        return_print(0, "while writing '%s' in cpufreq file (%s)", word, strerror(errno));
    }
    return 1;
}

int filemagic_word_mwrite(int *fds, int fds_count, char *word, int line_break)
{
    int len = strlen(word);
    int s = 1;
    int i = 0;
    debug("Multiple writing a word '%s'", word);
    // Adding a line break
    if (line_break) {
        word[len]   = '\n';
        word[len+1] = '0';
        len++;
    }
    for (i = 0; i < fds_count; ++i) {
        debug("Writting in fd%d", fds[i]);
        if (!filemagic_word_write(fds[i], word, len, 0)) {
            s = 0;
        }
    }
    return s;
}

int filemagic_once_read(char *path, char *buffer, int buffer_length)
{
    int fd = -1;
    int i  =  0;
    int r  =  0;
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
