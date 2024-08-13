/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/


int module_affinity()
{
	char *s = NULL;
	char *o = NULL;

    if ((s = ear_getenv("SLURM_LOCALID")) != NULL && (o = ear_getenv("EAR_AFFINITY")) != NULL)
    {
        cpu_set_t mask;
        int cpu = atoi(s);
        int off = atoi(o); 

        cpu = cpu * off;
        CPU_ZERO(&mask);
        CPU_SET(cpu, &mask);
        
        int result = sched_setaffinity(0, sizeof(cpu_set_t), &mask);
        verbose(0, "TASK %s: set to CPU %d (%d, %d, %s) %d", s, cpu, result, errno, strerror(errno), mask);
    }
}
