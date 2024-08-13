/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#ifndef EAR_DLB_TALP_LIB_H_
#define EAR_DLB_TALP_LIB_H_


#include <common/states.h>
#include <common/system/time.h>


typedef struct earl_talp_s
{
	timestamp_t timestamp;	
  float   parallel_efficiency;
  float   load_balance;
} earl_talp_t;


/** Module's init function.
 * It currently starts the internal monitor to collect DLB node metrics.
 * It must be called by all processes. */
state_t earl_dlb_talp_init();


/** Module's ending-point function. */
state_t earl_dlb_talp_dispose();


state_t earl_dlb_talp_read(earl_talp_t *earl_talp_data);


#endif  // EAR_DLB_TALP_LIB_H_
