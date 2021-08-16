#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <common/states.h>
#include <library/dynais/dynais.h>

topology_t topo;
dynais_call_t dynais;
uint panic;

int call(ushort sample)
{
	static uint c = 0;
	uint level;
	uint size;
	int stat;

	stat = dynais(sample, &size, &level);
	fprintf(stderr, "%u %u %d %u %u\n", c, sample, stat, level, size);

	if (c++ == panic) {
		exit(0);
	}
}

int main(int argc, char *argv[])
{
	ulong value[2];
	FILE *input;
	uint sample;
	state_t s;

	state_assert(s, topology_init(&topo),);

	if ((dynais = dynais_init(&topo, atoi(argv[1]), atoi(argv[2]))) == NULL) {
		fprintf(stderr, "dynais_init returned NULL\n");
		return 0;
	}

    if ((input = fopen(argv[3], "r")) < 0) {
        fprintf(stderr, "file read failed\n");
        return;
    }
    
	panic = UINT_MAX;
    if (argc > 4) panic = (uint) atoi(argv[4]);
	
	while (fread(&value, 2, sizeof(ulong), input) > 0)
    {
		sample = (ushort) value[0];
		call(sample);
	}

	return 0;
}
