/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef METRICS_COMMON_FILE_H
#define METRICS_COMMON_FILE_H

#define dont_open NULL

int filemagic_exists(char *path);

int filemagic_can_read(char *path, int *fd);

int filemagic_can_write(char *path, int *fd);

int filemagic_can_mread(char *format, int index_count, int **fds);

int filemagic_can_mwrite(char *format, int index_count, int **fds);

int filemagic_word_read(int fd, char *word, int reset_position);

int filemagic_word_write(int fd, char *word, int word_length, int line_break);

int filemagic_word_mwrite(int *fds, int fds_count, char *word, int line_break);

int filemagic_once_read(char *path, char *buffer, int buffer_length);

#endif // METRICS_COMMON_FILE_H
