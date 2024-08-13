/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/sizes.h>
#include <common/string_enhanced.h>

char *strtolow(char *string)
{
    char *p = string;
    while (*string != '\0') {
        *string = tolower((unsigned char) *string);
        string++;
    }
    return p;
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

void str_cut_list(char *src, char ***elements, int *num_elements, char *separator)
{
    if (src == NULL) {
        *elements = NULL;
        *num_elements = 0;
        return;
    }
    int tmp_num = 0;
    char **tmp_elements = calloc(1, sizeof(char *));
    tmp_elements[0] = strtok(src, separator);
    tmp_num++;
    char *token;
    
    while((token = strtok(NULL, separator)) != NULL)
    {
        tmp_elements = realloc(tmp_elements, sizeof(char *)*(tmp_num + 1));
        tmp_elements[tmp_num] = token;
        tmp_num++;
    }

    *elements = tmp_elements;
    *num_elements = tmp_num;

}
