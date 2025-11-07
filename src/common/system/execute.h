/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

#ifndef _EXECUTE_ACTIONS_H
#define _EXECUTE_ACTIONS_H
__attribute__((used)) int execute(char *cmd);
int execute_with_fork(char *cmd);
void print_stack(int fd);
void *get_stack(int lv);
#endif
