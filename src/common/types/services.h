/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

/** Creates a new /var/run/service.pid file with the pid of the calling process */
/* returns 1 if the service must recover the status because of failure */
int new_service(char *service);

/** Releases the /var/run/service.pid file */
void end_service(char *name);

