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

#ifndef _STATE_COMMON_H
#define _STATE_COMMON_H

#include <common/config.h>
#include <common/types/loop.h>

/* EARL states identifiers */
#define NO_PERIOD                   0
#define FIRST_ITERATION             1
#define EVALUATING_LOCAL_SIGNATURE  2
#define SIGNATURE_STABLE            3
#define PROJECTION_ERROR            4
#define RECOMPUTING_N               5
#define SIGNATURE_HAS_CHANGED       6
#define TEST_LOOP                   7
#define EVALUATING_GLOBAL_SIGNATURE 8

/** Reports to the EARL output channel the signature. This function takes into account EARL output
 * options such as if using EARL_VERBOSE_PATH to output only the signature of the master node, the node
 * signature for each node, or the signature for each local process.
 * \param[in] sig The loop signature.
 * \param[in] master_rank Whether the calling process is an EARL's master_rank.
 * \param[in] show TODO.
 * \param[in] aname TODO.
 * \param[in] nname TODO.
 * \param[in] iterations The number of iterations of the current application.
 * \param[in] prevf The CPU frequency of the previous computed signature.
 * \param[in] new_freq The next frequency applied.
 * \param[in] mode TODO.
 */
void state_verbose_signature(loop_t *sig, int master_rank, int show, char *aname,
        char *nname, int iterations, ulong prevf, ulong new_freq, char *mode);

void state_report_traces(int master_rank, int my_rank, int lid, loop_t *lsig, ulong freq, ulong status);
void state_report_traces_state(int master_rank, int my_rank,int lid, ulong status);
void state_print_policy_state(int master_rank, int st);
#endif

