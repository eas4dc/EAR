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

#ifndef EAR_POLICIES_STATES_H
#define EAR_POLICIES_STATES_H

#define EAR_POLICY_READY 		0
#define EAR_POLICY_CONTINUE 1 /* Frequency has not been changed and next state must be the same */
#define EAR_POLICY_GLOBAL_EV 2 /* Next state must be a global evaluation */
#define EAR_POLICY_GLOBAL_READY 3 /* Global evaluation ready */
#define EAR_POLICY_LOCAL_EV			4
#define EAR_POLICY_TRY_AGAIN 5 /* Frquency has been changed but next state must be the same */

#define EAR_POLICY_NO_STATE 1000
#endif //EAR_POLICIES_H
