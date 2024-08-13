/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


#include <common/config/config_def.h>
#include <common/states.h>
#include <common/types/types.h>
#include <common/types/configuration/cluster_conf.h>
#include <common/output/verbose.h>
#include <daemon/local_api/eard_api.h>
#include <report/report.h>


static uint must_report;
static char report_nodename[GENERIC_NAME];
static uint earl_report_loops = EARL_REPORT_LOOPS;

state_t report_init(report_id_t *id,cluster_conf_t *cconf)
{
	debug("eard report_init");
	if (id->master_rank >= 0) must_report = 1;
  gethostname(report_nodename, sizeof(report_nodename));
  char * cearl_report_loops = ear_getenv(FLAG_REPORT_LOOPS);
	if (cearl_report_loops != NULL) earl_report_loops = atoi(cearl_report_loops);

	debug("EARL loops will be reported ? %u", earl_report_loops);

  strtok(report_nodename, ".");

	return EAR_SUCCESS;
}

state_t report_applications(report_id_t *id,application_t *apps, uint count)
{
	int i;
	debug("eard report_applications");
	if (!must_report) return EAR_SUCCESS;
	if (!eards_connected()) return EAR_ERROR;
	if ((apps == NULL) || (count == 0)) return EAR_SUCCESS;
	for (i=0;i<count;i++){
		eards_write_app_signature(&apps[i]);
	}
	return EAR_SUCCESS;
}

state_t report_loops(report_id_t *id,loop_t *loops, uint count)
{
    int i;
#if SHOW_DEBUGS
    char sig_bufg[1024];
#endif
    debug("eard report_loops");
    if (!must_report) return EAR_SUCCESS;
    if (!eards_connected()) return EAR_ERROR;
    if ((loops == NULL) || (count == 0) || !earl_report_loops) return EAR_SUCCESS;
    for (i = 0; i < count; i++) {
#if SHOW_DEBUGS
        signature_to_str(&loops[i].signature, sig_bufg, sizeof(sig_bufg));
        verbose(0,"Loop_reported: %s", sig_bufg);
#endif
        eards_write_loop_signature(&loops[i]);
    }
    return EAR_SUCCESS;
}

state_t report_events(report_id_t *id, ear_event_t *eves, uint count)
{
	time_t curr = time(NULL);
	if (!must_report) return EAR_SUCCESS;
  if (!eards_connected()) return EAR_ERROR;

	debug("Reporting event");

	for (uint i = 0; i < count; i++){
  	eves[i].timestamp = curr;
  	eards_write_event(&eves[i]);
	}
	return EAR_SUCCESS;
}

state_t report_misc(report_id_t *id, uint type, const char *data, uint count)
{
	if (!must_report) return EAR_SUCCESS;
	if (!eards_connected()) return EAR_ERROR;
	application_t * apps = (application_t *)data;
	debug("Report type %u", type);
	for (uint i = 0; i < count; i++){
		if (type == WF_APPLICATION){
			eards_write_wf_app_signature(&apps[i]);
		}
  }
  return EAR_SUCCESS;

}
