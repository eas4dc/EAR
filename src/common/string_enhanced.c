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
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/sizes.h>
#include <common/colors.h>
#include <common/string_enhanced.h>

// tprintf
static const char *sym = STR_SYMBOL;
static const char *sym_vis = STR_SYMBOL_VIS;
static unsigned int format[STR_MAX_COLUMNS];
static unsigned int columns;
static unsigned int mode;
static int fd;


/*
 *
 *
 *
 */

static int tprintf_color_open(char **iput, char **oput)
{
	char *ibuf = *iput;
	char *obuf = *oput;

	#define is_color(tag, color) \
	if (strncmp(ibuf, tag, STR_COL_CHR) == 0) \
	{ \
		sprintf(obuf, "%s", color); \
		*iput = &ibuf[STR_COL_CHR]; \
		*oput = &obuf[COL_CHR]; \
		return 1; \
	}

	is_color(STR_RED, COL_RED);
	is_color(STR_GRE, COL_GRE);
	is_color(STR_YLW, COL_YLW);
	is_color(STR_BLU, COL_BLU);
	is_color(STR_MGT, COL_MGT);
	is_color(STR_CYA, COL_CYA);

	return 0;
}

static void tprintf_color_close(char **oput)
{
	char *obuf = *oput;

	sprintf(obuf, "%s", COL_CLR);
	*oput = &obuf[CLR_CHR];
}

/*
 *
 *
 *
 */

int tprintf_init(int _fd, int _mode, char *_format)
{
	int len = strlen(_format);
    char *tok;

	if (len >= STR_SIZE_BUFFER) {
		columns = 0;
		return -1;
	}

	//
	fd   = _fd;
	mode = _mode;
	columns = 0;

	// Getting the format
	strcpy(tprintf_ibuf, _format);

	//
	tok = strtok(tprintf_ibuf, " ");

	//
	while (tok != NULL && columns < STR_MAX_COLUMNS) {
		format[columns++] = atoi(tok);
		tok = strtok(NULL, " ");
	}

	if (columns >= STR_MAX_COLUMNS) {
		columns = 0;
		return -1;
	}

	return 0;
}

int tprintf_format()
{
    char *p1 = strstr(tprintf_ibuf, sym);
    char *p2 = tprintf_ibuf;
    char *p3 = tprintf_obuf;

    int len = strlen(tprintf_ibuf);
	int chr = 0;
	int col = 0;
	int vis = 0;
    int i = 0;

	if ((len >= STR_SIZE_BUFFER) || (columns == 0)) {
		return -1;
	}

    while(p1 && i < columns)
    {
		// If it is a visible wall
		if (vis)
		{
			p3[0] = '|';
            p3[1] = ' ';
            p3++;
            p3++;
			p2++;
		}

    	// If color
		if (mode == STR_MODE_COL) {
			col = tprintf_color_open(&p2, &p3);
		}

    	// Character copy and count
        while(p2 != p1) {
            *p3 = *p2;
            ++chr;
            ++p2;
            ++p3;
		}

        // Number of characters per column
        while(chr < format[i]) {
            *p3 = ' ';
            ++chr;
            ++p3;
        }

		if (col) {
			tprintf_color_close(&p3);
			col = 0;
		}

		if (p1 == &tprintf_ibuf[len]) {
			break;
		}

		// Visible wall
		vis = (strncmp(p2, sym_vis, strlen(sym_vis)) == 0);

        ++i;
        p1++;
        p1++;
        p2 = p1;
        p1 = strstr(p1, sym);

        if (!p1) p1 = &tprintf_ibuf[len];
		
        chr = 0;
    }

	p3[0] = '\n';
	p3[1] = '\0';
    dprintf(fd, "%s", tprintf_obuf);

	return 0;
}

void tprintf_close()
{
	columns = 0;
}

void strtolow(char *string)
{
    while (*string) {
        *string = tolower((unsigned char) *string);
        string++;
    }
}

void strtoup(char *string)
{
    while (*string) {
        *string = toupper((unsigned char) *string);
        string++;
    }
}

int strinlist(const char *list, const char *separator, const char *element)
{
	char buffer[SZ_PATH];
	char *e;

	strcpy(buffer, list);	
	e = strtok(buffer, separator);
	while (e != NULL) {
		// An extra parameter would be needed to switch between strcmp and strncmp.
		if (strncmp(element, e, strlen(e)) == 0) {
			return 1;
		}
		e = strtok(NULL, separator);
	}
	return 0;
}

int strinargs(int argc, char *argv[], const char *opt, char *value)
{
    int expected;
    int result;
	int auxind;
	int auxerr;
	size_t len;
    // Saving opt globals, and setting to 0 to prevent prints
	auxind = optind;
	auxerr = opterr;
    optind = 1;
    opterr = 0;
	// If the option argument is long
    if ((len = strlen(opt)) > 2)
	{
    	struct option long_options[2];
    	int has_arg = 2;
        //
        memset(long_options, 0, sizeof(struct option) * 2);
        // has_value is 2 by default because means optional, if not
        // has_value is 1, which means the argument is required.
		if (opt[len-1] == ':') {
            has_arg = 1;
		}
        // Filling long_options structure to return 2.
        long_options[0].name    = opt;
        long_options[0].has_arg = has_arg;
        long_options[0].flag    = NULL;
        long_options[0].val     = 2;
        // This function is for '-long' or '--long' type arguments, 
        // and returns 2 as we specified.
        do { 
        	result = getopt_long (argc, argv, "", long_options, NULL);
		} while (result != 2 && result != -1);
        // Expected result
        expected = 2;
        //int result2 = getopt_long (argc, argv, "", long_options, NULL);
    } else {
        // This function is for '-i' type parameters.
        result = getopt(argc, argv, opt);
        // Expected result
        expected = opt[0];
    }
    // Recovering opterr default value.
    opterr = auxerr;
    optind = auxind;
    // Copying option argument in value buffer. 
    if (value != NULL) {
        value[0] = '\0';
        if (optarg != NULL) {
            strcpy(value, optarg);
        }
    }
    // If the result value is the letter option.
    return (result == expected);
}

char* strclean(char *string, char chr)
{
    char *index = strchr(string, chr);
    if (index == NULL) return NULL;
    string[index - string] = '\0';
    return index;
}

void remove_chars(char *s, char c)
{
    int writer = 0, reader = 0;

    while (s[reader])
    {
        if (s[reader]!=c) 
            s[writer++] = s[reader];
        
        reader++;       
    }

    s[writer]=0;
}
