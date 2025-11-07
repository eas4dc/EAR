/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef COMMON_STRING_ENHANCED_H
#define COMMON_STRING_ENHANCED_H

#include <common/utils/strtable.h>
#include <getopt.h>

/** Cleans the character pointed by 'chr', adding an '\0' in its position. */
char *strclean(char *string, char chr);

/** Converts a string to lower case. */
char *strtolow(char *string);

/** Converts a string to upper case. */
void strtoup(char *string);

/** Given a list and a separator in strtok format, search an element in the list. */
int strinlist(const char *list, const char *separator, const char *element);

/** */
int strinargs(int argc, char *argv[], const char *opt, char *value);

/** Removes characters c from string s. */
void remove_chars(char *s, char c);

/** Removes any character found in removable from string s. */
void remove_chars_list(char *s, char *removable);

/** Parses src into a list of char* elements with n num_elements separated by separator in src. */
void str_cut_list(char *src, char ***elements, int *num_elements, char *separator);

/** Replaces all the occurences of old_char in *string by new_char.
 * For example, replace_char("hello world", 'l', 'm') generates "hemmo wormd". */
void replace_char(char *string, char old_char, char new_char);

#endif
