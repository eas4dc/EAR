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

#ifndef EAR_EARDBD_SIGNALS_H
#define EAR_EARDBD_SIGNALS_H

#include <signal.h>

#define edb_error(...) \
	print_line(0); \
	verbose(0, "ERROR, " __VA_ARGS__); \
	error_handler();

void log_handler(cluster_conf_t *conf_clus, uint close_previous);

void signal_handler(int signal, siginfo_t *info, void *context);

void error_handler();

#endif //EAR_EARDBD_SIGNALS_H
