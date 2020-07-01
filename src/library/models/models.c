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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <common/config.h>
#include <common/states.h>
#include <common/output/verbose.h>
#include <common/hardware/architecture.h>
#include <common/types/projection.h>
#include <common/types/application.h>
#include <library/common/externs.h>
#include <library/models/models.h>

static int use_default=1;



// Normals
//
static uint reset_freq_opt = RESET_FREQ;
static uint ear_models_pstates = 0;
static ulong user_selected_freq;
static int model_nominal=1;


state_t init_power_models(uint user_type,conf_install_t *data,architecture_t *arch_desc)
{
	state_t st;
	st=projections_init(user_type,data,arch_desc);
	ear_models_pstates = arch_desc->pstates;
	projection_create(arch_desc->pstates);
	projection_reset(arch_desc->pstates);
	return EAR_SUCCESS;
}



