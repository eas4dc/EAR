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

#include <stdio.h>
#include <string.h>
#include <common/utils/args.h>
#include <common/utils/string.h>

int args_get(int argc, char *argv[], const char *arg_in, char *buffer)
{
	int is_required = 0;
	int next_value  = 0;
	int curr_value  = 0;
	int has_equal   = 0;
	int is_short    = 0;
	int is_long     = 0;

	char arg[128];
	char aux[128];
	char *p;
	int i;

	if (strlen(arg_in) == 0) {
		return 0;
	}
	// Saving the argument in the stack.
	strcpy(aux, arg_in);
	// If value is mandatory
	if (aux[strlen(aux)-1] == ':') {
		aux[strlen(aux)-1] = '\0';
		is_required = 1;
	}
	// Is long or short?
	is_short = (strlen(aux) == 1);
	is_long  = (strlen(aux) >= 2);
	//
	if (is_short) xsnprintf(arg, sizeof(arg),  "-%s", aux);
	if (is_long)  xsnprintf(arg, sizeof(arg), "--%s", aux);
	//
	if (buffer != NULL) {
		buffer[0] = '\0';
	}
	//
	for (i = 0; i < argc; ++i)
	{
		if (strncmp(argv[i], arg, strlen(arg)) != 0) {
			continue;
		}
		// Is equal, check if has value
		curr_value = (strlen(argv[i]) > strlen(arg));
		// Check if next is value
		next_value = (i < (argc-1)) && (
					 (strncmp(argv[i+1], "--", 2) != 0) &&
					 (strncmp(argv[i+1],  "-", 1) != 0) );
		// Check if has equal symbol (=)
		has_equal = (strchr(argv[i], '=') != NULL);
		//
		if (is_short && curr_value && !has_equal && buffer) {
			p = &argv[i][strlen(arg)];
			strcpy(buffer, p);
			return 1;
		}
		if (is_short && next_value && buffer) {
			strcpy(buffer, argv[i+1]);
			return 1;
		}
		if (is_long && curr_value && has_equal && buffer) {
			p = &argv[i][strlen(arg)+1];
			strcpy(buffer, p);
			return 1;
		}
		if (is_long && next_value) {
			strcpy(buffer, argv[i+1]);
			return 1;
		}
		if ((is_long || is_short) && !is_required) {
			// True if value is not mandatory and both flags are completely equal
			if (strcmp(argv[i], arg) == 0) {
				return 1;
			}
		}
	}
	return 0;
}
