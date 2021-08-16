static freq_cpu_t freq1;
static freq_cpu_t freq2;
static ulong freqs_average;
static ulong freqs_count;
static ulong *freqs;
static ulong cpu_count;
static topology_t topo;
static uint half;

void freq_monitor_init()
{
	state_t s;
    state_assert(s, topology_init(&topo), exit(0));
    state_assert(s, freq_cpu_init(&topo), exit(0));
    state_assert(s, freq_cpu_data_alloc(&freq1, NULL  , NULL        ), exit(0));
    state_assert(s, freq_cpu_data_alloc(&freq2, &freqs, &freqs_count), exit(0));
	state_assert(s, freq_cpu_read(&freq1), exit(0));
    half = ((uint) freqs_count) / 2;
}

void freq_monitor_init()
{
    double dfreq;
	state_t s;
    uint cpu;

	state_assert(s, freq_cpu_read_copy(&freq2, &freq1, freqs, &freqs_average), exit(0));

	for (cpu = 0; cpu < half; ++cpu) {
		dfreq = (((double) freqs[cpu]) + ((double) freqs[cpu+half])) / 2000000.0;
		fprintf(stderr, "%0.1lf ", dfreq);
	}
	dfreq = ((double) freqs_average) / 1000000.0;
	fprintf(stderr, "| %0.2lf\n", dfreq);

    return 0;
}
