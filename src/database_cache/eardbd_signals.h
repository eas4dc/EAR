/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef EAR_EARDBD_SIGNALS_H
#define EAR_EARDBD_SIGNALS_H

#include <signal.h>

#define edb_error(...)                                                                                                 \
    print_line(0);                                                                                                     \
    verbose(0, "ERROR, " __VA_ARGS__);                                                                                 \
    error_handler();

void log_handler(cluster_conf_t *conf_clus, uint close_previous);

void signal_handler(int signal, siginfo_t *info, void *context);

void error_handler();

#endif // EAR_EARDBD_SIGNALS_H
