/*
 *
 * This program is part of the EAR software.
 *
 * EAR provides a dynamic, transparent and ligth-weigth solution for
 * Energy management. It has been developed in the context of the
 * Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
 *
 *
 * BSC Contact   mailto:ear-support@bsc.es
 *
 *
 * EAR is an open source software, and it is licensed under both the BSD-3 license
 * and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
 * and COPYING.EPL files.
 */

#pragma once

#include <stdint.h>
#include <stdio.h>

/* Signature values */
#define FREQ_WIDTH         8
#define IMC_WIDTH          8
#define GBS_WIDTH          7
#define IO_WIDTH           7
#define MPI_WIDTH          8
#define CPI_WIDTH          5
#define TIME_WIDTH         10
#define FLOPS_WIDTH        10 // must be divisible by 2
#define POWER_WIDTH        10
#define CACHE_WIDTH        10
#define TPI_WIDTH          8
#define GFLOPS_WIDTH       10
#define CYCLES_WIDTH       10
#define INSTRUCTIONS_WIDTH 10
#define PCK_POWER_WIDTH    10
#define DRAM_POWER_WIDTH   10
#define ENERGY_WIDTH       12
#define VPI_WIDTH          14
#define CARBON_WIDTH       20

/* Job an application values */
#define APP_WIDTH   16
#define OTHER_WIDTH 10

#define APP_TYPE    100001
#define LOOP_TYPE   100002

/* Format values */
typedef enum signature_format_type {
    FREQUENCY        = 'f',
    TIME             = 't',
    ENERGY           = 'e',
    DC_POWER         = 'p',
    DRAM_POWER       = 'D',
    PCK_POWER        = 'P',
    IMC_FREQ         = 'i',
    GBS              = 'g',
    MPI_PERC         = 'm',
    IO_MBS           = 'o',
    FLOPS            = 'F',
    GPU              = 'G',
    ALL_FREQS        = 'r',
    CACHE_MISSES     = 'L',
    CPI              = 'C',
    DEF_FREQ         = 'd',
    TPI              = 'T',
    GFLOPS           = 's',
    CYCLES           = 'y',
    INSTRUCTIONS     = 'I',
    VPI              = 'v',
    CARBON_FOOTPRINT = 'B',
} signature_format_type;

typedef enum job_format_type {
    APP_ID    = 'a',
    JOB_WSTEP = 'j',
    USER_ID   = 'u',
    NODE_ID   = 'n',
    NUM_NODES = 'N',
    LOCAL_ID  = 'x',
    DATE_ITER = 'X',
} job_format_type;

static int32_t out_format_fd = STDOUT_FILENO;

/* Auxiliary print functions */
void print_format_lf(int WIDTH, double value, int float_precision)
{
    dprintf(out_format_fd, "%-*.*lf ", WIDTH, float_precision, value);
}

void print_format_lu(int WIDTH, unsigned long value)
{
    dprintf(out_format_fd, "%-*lu ", WIDTH, value);
}

void print_format_dc(int WIDTH, int32_t value)
{
    dprintf(out_format_fd, "%-*d ", WIDTH, value);
}

void print_format_llu(int WIDTH, unsigned long long value)
{
    dprintf(out_format_fd, "%-*llu ", WIDTH, value);
}

void print_format_str(int WIDTH, char *value)
{
    dprintf(out_format_fd, "%-*s ", WIDTH, value);
}

/* Generic print macro. It calls the appropiate function depending on the
 * type of the 'value' argument, simplifying the process.
 * __VA_OPT__(,) is compiler dependant, but both GCC and CLANG (and thus ICX) implement it.
 */
#define print_format(WIDTH, value, ...)                                                                                \
    _Generic((value),                                                                                                  \
        double: print_format_lf,                                                                                       \
        float: print_format_lf,                                                                                        \
        int32_t: print_format_dc,                                                                                      \
        uint32_t: print_format_lu,                                                                                     \
        uint64_t: print_format_lu,                                                                                     \
        unsigned long long: print_format_llu,                                                                          \
        char *: print_format_str,                                                                                      \
        default: print_format_str)(WIDTH, value __VA_OPT__(, ) __VA_ARGS__)

#define print_format_custom(...) dprintf(out_format_fd, __VA_ARGS__)

#define new_line_format()        dprintf(out_format_fd, "\n")
