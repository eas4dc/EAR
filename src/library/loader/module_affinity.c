int module_affinity()
{
	char *s = NULL;
	char *o = NULL;

    if ((s = getenv("SLURM_LOCALID")) != NULL && (o = getenv("EAR_AFFINITY")) != NULL)
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
