/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

int lock_master(char *path_tmp, int job_id);

int master_getstep(int job_id, int step_id);

void unlock_slave(int job_id, int step_id);

void spinlock_slave(int job_id);

int slave_getstep(int job_id, int step_id);

void files_clean(int job_id, int step_id);

void folder_clean(int job_id);

void all_clean(char *path_tmp);
