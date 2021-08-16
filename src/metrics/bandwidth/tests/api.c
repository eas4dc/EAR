#include <unistd.h>
#include <common/output/verbose.h>
#include <metrics/bandwidth/bandwidth.h>

ullong *cas1;
ullong *cas2;
ullong *cas3;

int main(int argc, char *argv[])
{
	ullong t;
	double d;
	int c;
	int r;
	int i;

	c = init_uncores(0);
	verbose(0, "init_uncores: %d", c);
	alloc_uncores(&cas1, c);
	alloc_uncores(&cas2, c);
	alloc_uncores(&cas3, c);
	r = start_uncores();
	verbose(0, "start_uncores: %d", r);
	r = read_uncores(cas1);
	verbose(0, "read_uncores: %d", r);
	print_uncores(cas1, c);
	sleep(3);
	r = stop_uncores(cas2);
	verbose(0, "stop_uncores: %d", r);
	print_uncores(cas2, c);
	diff_uncores(cas3, cas2, cas1, c);
	verbosen(0, "diff_uncores: ");
	//for (i = 0, t = 0LLU; i < c; ++i) {
	//	t += cas[i];
	//	verbosen(0, "%llu ", cas[i]);
	//}
	verbose(0, " ");
	r = compute_mem_bw(cas2, cas1, &d, 3.0, c);
	verbose(0, "compute_mem_bw: %d", r);
	//t = (t * 64LLU) / (3LLU * BW_MB);
	//verbose(0, "total bandwidth: %llu MB/s", t);
	verbose(0, "total bandwidth: %0.2lf GB/s", d);

	return 0;
}
