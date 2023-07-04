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

#ifndef METRICS_COMMON_FILE_H
#define METRICS_COMMON_FILE_H

#define dont_open NULL

int filemagic_can_read(char *path, int *fd);

int filemagic_can_write(char *path, int *fd);

int filemagic_can_mread(char *format, int index_count, int **fds);

int filemagic_can_mwrite(char *format, int index_count, int **fds);

int filemagic_word_read(int fd, char *word, int reset_position);

int filemagic_word_write(int fd, char *word, int s, int line_break);

int filemagic_word_mwrite(int *fds, int fds_count, char *word, int line_break);

int filemagic_once_read(char *path, char *buffer, int buffer_length);

#endif //METRICS_COMMON_FILE_H
