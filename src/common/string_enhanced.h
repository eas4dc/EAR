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

#ifndef _STRING_ENHANCED_H_
#define _STRING_ENHANCED_H_

#include <stdio.h>
#include <getopt.h>
#include <linux/limits.h>
#include <common/utils/string.h>
#include <common/output/verbose.h>

#define STR_MAX_COLUMNS		20
#define STR_SIZE_BUFFER		PIPE_BUF
#define STR_SYMBOL			"||"
#define STR_SYMBOL_VIS		"|||"
#define STR_RED				"<red>"
#define STR_GRE 			"<gre>"
#define STR_YLW 			"<ylw>"
#define STR_BLU 			"<blu>"
#define STR_MGT 			"<mgt>"
#define STR_CYA 			"<cya>"
#define STR_COL_CHR			5
#define STR_MODE_DEF		0
#define STR_MODE_COL		1
#define STR_MODE_CSV		2

char tprintf_ibuf[STR_SIZE_BUFFER] __attribute__((weak));
char tprintf_obuf[STR_SIZE_BUFFER] __attribute__((weak));

#define tprintf(...) \
	snprintf(tprintf_ibuf, STR_SIZE_BUFFER-1, __VA_ARGS__); \
	tprintf_format();

/** **/
int tprintf_init(int fd, int mode, char *format);

/** Using this version the output is redirected to verbose. */
int tprintf_init_v2(int v, int mode, char *format);

/** **/
int tprintf_format();

/** **/
void tprintf_close();

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
