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

#ifndef COMMON_STRING_ENHANCED_H
#define COMMON_STRING_ENHANCED_H

#include <getopt.h>
#include <common/utils/strtable.h>

/** Cleans the character pointed by 'chr', adding an '\0' in its position. */
char* strclean(char *string, char chr);

/** Converts a string to lower case. */
void strtolow(char *string);

/** Converts a string to upper case. */
void strtoup(char *string);

/** Given a list and a separator in strtok format, search an element in the list. */
int strinlist(const char *list, const char *separator, const char *element);

/** */
int strinargs(int argc, char *argv[], const char *opt, char *value);

/** Removes characters c from string s. */
void remove_chars(char *s, char c);

/** Parses src into a list of char* elements with n num_elements separated by separator in src. */
void str_cut_list(char *src, char ***elements, int *num_elements, char *separator);

#endif
