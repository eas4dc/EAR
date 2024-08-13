/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

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
void state_verbose_signature(loop_t *sig, int master_rank, char *aname, char *nname,
														 int iterations, ulong prevf, ulong new_freq, char *mode);

void state_report_traces(int master_rank, int my_rank, int lid, loop_t *lsig, ulong freq, ulong status);
void state_report_traces_state(int master_rank, int my_rank,int lid, ulong status);
void state_print_policy_state(int master_rank, int st);

/** Sets \p perf_accuracy_min_time and \p lib_period variables.
 * Order of precedence:
 * - environment variable EARL_TIME_LOOP_SIGNATURE
 * - ear.conf's LibraryPeriod field.
 * The value of \p perf_accuracy_min_time sets a lower bound for \p lib_period.
 * The value returned by eards_node_energy_frequency sets the lowerest bound. */
void states_comm_configure_performance_accuracy(cluster_conf_t *cluster_conf, ulong *hw_perf_acc, uint *library_period);
#endif

