/***************************************************************************
 * Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 **************************************************************************/

// #define SHOW_DEBUGS 1

#include <common/math_operations.h>
#include <common/output/debug.h>
#include <metrics/cpufreq/archs/dummy.h>
#include <metrics/cpufreq/archs/eard.h>
#include <metrics/cpufreq/archs/intel63.h>
#include <metrics/cpufreq/cpufreq.h>
#include <pthread.h>
#include <stdlib.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static cpufreq_ops_t ops;
static apinfo_t info;
static cpufreq_base_t bf;

void cpufreq_load(topology_t *tp, int options)
{
    while (pthread_mutex_trylock(&lock))
        ;
    if (info.api != API_NONE) {
        goto leave;
    }
    if (API_IS(options, API_DUMMY)) {
        goto dummy;
    }
    cpufreq_intel63_load(tp, &ops, options);
    cpufreq_eard_load(tp, &ops, options);
dummy:
    cpufreq_dummy_load(tp, &ops, options);
    cpufreq_get_info(&info);
    // Getting base frequency
    cpufreq_base_init(tp, &bf);
leave:
    pthread_mutex_unlock(&lock);
}

void cpufreq_unload()
{
    while (pthread_mutex_trylock(&lock))
        ;
    if (info.api != API_NONE) {
        ops.unload();
        memset(&ops, 0, sizeof(cpufreq_ops_t));
        memset(&info, 0, sizeof(apinfo_t));
    }
    pthread_mutex_unlock(&lock);
}

void cpufreq_get_info(apinfo_t *info)
{
    memset(info, 0, sizeof(apinfo_t));
    info->layer = "CPUFREQ";
    info->api   = API_NONE;
    if (ops.get_info != NULL) {
        ops.get_info(info);
    }
}

state_t cpufreq_read(cpufreq_t *f)
{
    preturn(ops.read, f);
}

state_t cpufreq_read_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *fD, ulong *fA)
{
    state_t s;
    if (state_fail(s = cpufreq_read(f2))) {
        return s;
    }
    cpufreq_data_diff(f2, f1, fD, fA);
    return s;
}

state_t cpufreq_read_copy(cpufreq_t *f2, cpufreq_t *f1, ulong *fD, ulong *fA)
{
    state_t s;
    if (state_fail(s = cpufreq_read_diff(f2, f1, fD, fA))) {
        return s;
    }
    cpufreq_data_copy(f1, f2);
    return s;
}

void cpufreq_data_diff(cpufreq_t *f2, cpufreq_t *f1, ulong *fD, ulong *fA)
{
    ulong valid_count = 0;
    ulong mperf_diff;
    ulong aperf_diff;
    ulong aperf_pcnt;
    ulong freq_aux;
    int d;

    if (f2 == NULL || f1 == NULL) {
        return;
    }
    if (fA != NULL) {
        fA[0] = (f2[0].state == 2) ? bf.frequency : 0;
    }
    for (d = 0; d < info.devs_count; ++d) {
        if (fD != NULL) {
            fD[d] = bf.frequency;
        }
        if (f2[d].state || f1[d].state) {
            continue; // Read or Dummy error, do not continue
        }
        mperf_diff = overflow_magic_u64((ullong) f2[d].freq_mperf, (ullong) f1[d].freq_mperf, MAXBITS64);
        aperf_diff = overflow_magic_u64((ullong) f2[d].freq_aperf, (ullong) f1[d].freq_aperf, MAXBITS64);
#if 1
        debug("CPU%d: APERF %lu - %lu = %lu, MPERF %lu - %lu = %lu", d, f2[d].freq_aperf, f1[d].freq_aperf, aperf_diff,
              f2[d].freq_mperf, f1[d].freq_mperf, mperf_diff);
#endif
        if (aperf_diff == 0 || mperf_diff == 0) {
            continue;
        }
        // The aperf percentage function includes a multiplication per 100. The
        // only way to overflow that counter is measuring if the aperf
        // difference is smaller than the maximum ulong value ((ulong) -1LU)
        // divided by 100. In case it is, removing 7 bits prevents that problem.
        // But it is an incorrect solution because it would take a lot of bits
        // and does not control when the first APERF/MPERF is greater the
        // second. It has to be fixed.
        if (((ulong) (-1LU) / 100LU) < aperf_diff) {
            aperf_diff >>= 7;
            mperf_diff >>= 7;
        }
        // With the percentage applied to the base frequency, finally can be
        // computed the average frequency of a specific CPU.
        aperf_pcnt = (aperf_diff * 100LU) / mperf_diff;
        freq_aux   = (bf.frequency * aperf_pcnt) / 100LU;
        if (fD != NULL)
            fD[d] = freq_aux;
        if (fA != NULL)
            fA[0] += freq_aux;
        valid_count += 1LU;
    }
    if (fA != NULL && valid_count > 0LU) {
        fA[0] = fA[0] / valid_count;
    }
}

void cpufreq_data_alloc(cpufreq_t **f, ulong **freqs)
{
    if (f != NULL) {
        *f = calloc(info.devs_count, sizeof(cpufreq_t));
    }
    if (freqs != NULL) {
        *freqs = calloc(info.devs_count, sizeof(ulong));
    }
}

void cpufreq_data_copy(cpufreq_t *dst, cpufreq_t *src)
{
    memcpy(dst, src, sizeof(cpufreq_t) * info.devs_count);
}

void cpufreq_data_free(cpufreq_t **f, ulong **freqs)
{
    if (freqs != NULL && *freqs != NULL) {
        free(*freqs);
        *freqs = NULL;
    }
    if (f != NULL && *f != NULL) {
        free(*f);
        *f = NULL;
    }
}

void cpufreq_data_print(ulong *freqs, ulong average, int fd)
{
    char buffer[SZ_BUFFER];
    cpufreq_data_tostr(freqs, average, buffer, sizeof(buffer));
    dprintf(fd, "%s", buffer);
}

char *cpufreq_data_tostr(ulong *freqs, ulong average, char *buffer, size_t length)
{
    static int mod = 8;
    double freq_ghz;
    int acc = 0;
    int i;

    if (info.devs_count >= 128) {
        mod = 16;
    }
    for (i = 0; i < info.devs_count; ++i) {
        if ((i != 0) && (i % mod == 0)) {
            acc += sprintf(&buffer[acc], "\n");
        }
        freq_ghz = ((double) freqs[i]) / 1000000.0;
        acc += sprintf(&buffer[acc], "%0.1lf ", freq_ghz);
    }
    freq_ghz = ((double) average) / 1000000.0;
    acc += sprintf(&buffer[acc], "!%0.1lf", freq_ghz);
    // acc += sprintf(&buffer[acc], " (GHz)\n");
    return buffer;
}

#if TEST
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static topology_t tp;
static apinfo_t info;
static cpufreq_t t1[16];
static cpufreq_t t2[16];
static ulong tD[16];
static ulong tA;
static pid_t pid;
static int forked;

static int count_fds()
{
    int dummy_fd = open("/dev/null", O_RDONLY);
    close(dummy_fd);
    return dummy_fd;
}

int main(int argc, char *argv[])
{
    topology_init(&tp);
reload:
    dprintf(STDOUT_FILENO, "%d: Loading... (%d fds)\n", getpid(), count_fds());
    cpufreq_load(&tp, SCOPE_PROCESS | API_FREE);
    cpufreq_get_info(&info);
    apinfo_tostr(&info);
    dprintf(STDOUT_FILENO, "%d: Loaded %s with %d fds\n", getpid(), info.api_str, count_fds());
reread:
    cpufreq_read(t1);
    sleep(3);
    int i, j;
    for (i = j = 0; i < 1000000; ++i) {
        j += 1;
    }
    cpufreq_read_copy(t2, t1, tD, &tA);
    dprintf(STDOUT_FILENO, "%d Printing...\n", getpid());
    cpufreq_data_print(tD, tA, STDOUT_FILENO);
    if (!forked) {
        sleep(1);
        pid    = fork();
        forked = 1;
        if (pid == 0) {
            sleep(1);
            dprintf(STDOUT_FILENO, "%d: Unloading...\n", getpid());
            cpufreq_unload();
            goto reload;
        }
    }
    goto reread;
    return 0;
}
#endif
