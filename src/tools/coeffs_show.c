/**************************************************************
*	Energy Aware Runtime (EAR)
*	This program is part of the Energy Aware Runtime (EAR).
*
*	EAR provides a dynamic, transparent and ligth-weigth solution for
*	Energy management.
*
*    	It has been developed in the context of the Barcelona Supercomputing Center (BSC)-Lenovo Collaboration project.
*
*       Copyright (C) 2017
*	BSC Contact 	mailto:ear-support@bsc.es
*	Lenovo contact 	mailto:hpchelp@lenovo.com
*
*	EAR is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	EAR is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with EAR; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*	The GNU LEsser General Public License is contained in the file COPYING
*/

#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <common/system/file.h>
#include <common/sizes.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/types/coefficient.h>

//static char buffer[SZ_PATH];

void print_coefficients(coefficient_t *avg, int n_pstates)
{
	int i;

	for (i=0;i<n_pstates;i++){
		coeff_print(&avg[i]);
	}
}

int main(int argc, char *argv[])
{
	coefficient_t *coeffs;
	state_t state;
	int n_pstates;
	int size;

    if (argc < 2) {
        verbose(0, "Usage: %s coeffs_file",argv[0]);
        exit(1);
    }
	VERB_SET_LV(5);
	size = file_size(argv[1]);

	if (size < 0) {
		error("invalid coeffs path %s (%s)", argv[1], intern_error_str);
		exit(1);
	}

    n_pstates = size / sizeof(coefficient_t);
	verbose(0, "coefficients file size: %d", size);
	verbose(0, "number of P_STATES: %d", n_pstates);

    coeffs = (coefficient_t*) calloc(size ,1);

    if (coeffs == NULL) {
		error("not enough memory");
		exit(1);
    }

    /* The program reports coefficients in stdout and csv file */
	state = file_read(argv[1], (char *) coeffs, size);

	if (state_fail(state)) {
		error("state id: %d (%s)", state, state_msg);

		exit(1);
	}

    print_coefficients(coeffs, n_pstates);

    return 0;
}
