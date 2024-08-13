/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _LIBRARY_LOADER_CONSTURCTOR_H_
#define _LIBRARY_LOADER_CONSTURCTOR_H_

int module_constructor(char *path_lib_so,char *libhack);

void module_destructor();

#endif // _LIBRARY_LOADER_CONSTURCTOR_H_
