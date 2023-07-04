/*
*
* This program is part of the EAR software.
*
* EAR provides a dynamic, transparent and ligth-weigth solution for
* Energy management. It has been developed in the context of the
* Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
*
* Copyright © 2017-present BSC-Lenovo
* BSC Contact   mailto:ear-support@bsc.es
* Lenovo contact  mailto:hpchelp@lenovo.com
*
* This file is licensed under both the BSD-3 license for individual/non-commercial
* use and EPL-1.0 license for commercial use. Full text of both licenses can be
* found in COPYING.BSD and COPYING.EPL files.
*/

#ifndef METRICS_COMMON_POPEN_H
#define METRICS_COMMON_POPEN_H

#include <common/states.h>

// Use example:
//  float f1, f2;
//  popen_t p;
//  int gpu;
//
//  popen_open("for i in {1..100}; do echo GPU $i 10.0 20.0 && sleep 1; done", 0, &p);
//  while(popen_read(&p, "aiff", &gpu, &f1, &f2)) {
//      printf("RETURNED: %d %f %f\n", gpu, f1, f2);
//  }
//  popen_close(&p); // You can close, or wait and try again to read
//
//  Format options:
//  - a: avoid (pass to next read argument)
//  - s: return integer
//  - i: return integer
//  - d: return double
//  - D: return double vector
//
//  Open options:
//  - escape_lines: avoids parsing and returning the first N typical header lines.
//  - one_shot: used when the popen process is closed after the reading and not
//              kept in the background. Under the hood blocks the read call,
//              ensuring you have something to read.

typedef struct popen_s {
    char  table[16][16][128];
    int   rows_escape;
    int   rows_index;
    int   rows_count;
    int   cols_count;
    FILE *file;
    int   eof;
    int   fd;
} popen_t;

// Tests if a executable (binary or script) exists, before popen_open.
state_t popen_test(char *exec);

state_t popen_open(char *command, int escape_lines, int one_shot, popen_t *p);

void popen_close(popen_t *p);

int popen_read(popen_t *p, const char* fmt, ...);

#endif //METRICS_COMMON_POPEN_H
