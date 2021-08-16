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

#ifndef REPORT_H
#define REPORT_H

#include <common/states.h>
#include <common/types/types.h>

// The API
//
// This API is designed to load plugin libraries to report some information to
// EARD, to EARDBD, to different databases and what the programmer wants, such
// as text files. You have to pass an installation path and a list of library
// files sepparated by ':'. If installation path is NULL, it will use the
// current path. If not, it will search in INSTALL_PATH/lib/plugins/report/
// folder. The library files have to include the '.so' extension.
//
// Props:
//	- Thread safe: yes.
//	- User mode: yes, if the plugin does user operations.
//------------------------------------------------------------------------------

typedef const char cchar;

typedef struct report_ops_s
{
	state_t (*report_init)             ();
	state_t (*report_dispose)          ();
	state_t (*report_applications)     (application_t *apps, uint count);
	state_t (*report_loops)            (loop_t *loops, uint count);
	state_t (*report_events)           (ear_event_t *eves, uint count);
	state_t (*report_periodic_metrics) (periodic_metric_t *mets, uint count);
	state_t (*report_misc)             (cchar *type, cchar *data, uint count);
} report_ops_t;

state_t report_load(const char *install_path, const char *libs);

state_t report_init();
state_t report_dispose();
state_t report_applications(application_t *apps, uint count);

state_t report_loops(loop_t *loops, uint count);

state_t report_events(ear_event_t *eves, uint count);

state_t report_periodic_metrics(periodic_metric_t *mets, uint count);

state_t report_misc(const char *type, const char *data, uint count);

#endif //REPORT_H
