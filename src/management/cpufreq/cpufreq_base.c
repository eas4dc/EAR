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

#include <common/environment_common.h>
#include <common/hardware/bithack.h>
#include <common/hardware/cpuid.h>
#include <common/hardware/defines.h>
#include <common/output/debug.h>
#include <common/system/time.h>
#include <common/utils/stress.h>
#include <management/cpufreq/cpufreq_base.h>
#include <math.h>
#include <metrics/common/file.h>
#include <metrics/common/msr.h>
#include <stdlib.h>
#include <unistd.h>

// Taken from arch/amd17.c
#define REG_HWCONF 0xc0010015
#define REG_P0     0xc0010064
// Taken from drivers/acpi_cpufreq.c and drivers/intel_pstate.c
#define FILE_SAF "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies"
#define FILE_CMF "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"
#define FILE_SMF "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
#define FILE_BAF "/sys/devices/system/cpu/cpu0/cpufreq/base_frequency"
#define FILE_CPB "/sys/devices/system/cpu/cpu0/cpufreq/cpb"
#define FILE_BST "/sys/devices/system/cpu/cpufreq/boost"
#define FILE_NTB "/sys/devices/system/cpu/intel_pstate/no_turbo"

static topology_t     tp_static;
static cpufreq_base_t bs_static;
static int            found_boost;
static int            found_freq;
static char           reason[256];

static int basefreq_completed()
{
    return (found_freq && found_boost);
}

static int basefreq_found_set(int f, int b)
{
    if (f)
        found_freq = 1;
    if (b)
        found_boost = 1;
    return basefreq_completed();
}

static void basefreq_clean()
{
    found_freq  = 0;
    found_boost = 0;
    sprintf(reason, "-");
}

static int basefreq_intel_haswell(topology_t *tp, cpufreq_base_t *base)
{
    // This is a complete function, it returns the frequency and boost.
    // There are no points to think that retrieving one of them could fail.
    if (tp->vendor != VENDOR_INTEL || tp->model < MODEL_HASWELL_X) {
        sprintf(reason, "Not Intel nor Haswell or greater");
        return 0;
    }
    cpuid_regs_t regs;
    // Getting base frequency (CPUID 16H)
    if (!found_freq) {
        CPUID(regs, 0x16, 0);
        // Computing base frequency in KHz
        if ((base->frequency = (regs.eax & 0x0000FFFF) * 1000LU) == 0) {
            sprintf(reason, "EAX@16H returned 0");
            // We don't trust on boost if this is 0
            return 0;
        }
    }
    // Getting boost status (CPUID 6H)
    if (!found_boost) {
        CPUID(regs, 0x06, 0);
        base->boost_enabled = (ulong) cpuid_getbits(regs.eax, 1, 1);
    }
    return basefreq_found_set(1, 1);
}

static int basefreq_amd_zen(topology_t *tp, cpufreq_base_t *base)
{
    ullong did, fid, mul;
    ullong aux;
    // This method requires permissions to, at least, read the MSR files.
    // If doesn't have enough privileges or the MSR driver is not present,
    // it will fail. In the future, maybe we can build an MSR class to avoid
    // the use of MSR driver.
    if (tp->vendor != VENDOR_AMD || tp->model < FAMILY_ZEN) {
        sprintf(reason, "Not AMD nor Zen or greater");
        return 0;
    }
    if (state_fail(msr_open(0, MSR_RD))) {
        sprintf(reason, "MSR can not be opened: %s", state_msg);
        return 0;
    }
    if (!found_freq) {
        if (state_ok(msr_read(0, (char *) &aux, sizeof(ullong), REG_P0))) {
            fid = getbits64(aux,  7, 0);
            did = getbits64(aux, 13, 8);
            mul = 200;
            if (tp->family >= FAMILY_ZEN5) {
                fid = getbits64(aux, 11, 0);
                did = 1LLU;
                mul = 5LLu;
            }
            if (did > 0) {
                base->frequency = (ulong) (((fid * mul) / did) * 1000);
                basefreq_found_set(1, 0);
            }
        } else {
            sprintf(reason, "MSR error while reading frequency: %s", state_msg);
        }
    }
    if (!found_boost) {
        if (state_ok(msr_read(0, (char *) &aux, sizeof(ullong), REG_HWCONF))) {
            base->boost_enabled = (uint) !getbits64(aux, 25, 25);
            basefreq_found_set(0, 1);
        } else {
            sprintf(reason, "MSR error while reading boost: %s", state_msg);
        }
    }
    return basefreq_completed();
}

static int basefreq_environment(topology_t *tp, cpufreq_base_t *base)
{
    char *freq  = ear_getenv("BASE_FREQ");
    char *boost = ear_getenv("BASE_BOOST");
    if (freq != NULL) {
        base->frequency = (ullong) atoi(freq);
        basefreq_found_set(1, 0);
    } else {
        sprintf(reason, "BASE_FREQ and/or BASE_BOOST are not set");
    }
    if (boost != NULL) {
        base->boost_enabled = (uint) atoi(boost);
        basefreq_found_set(0, 1);
    } else {
        sprintf(reason, "BASE_FREQ and/or BASE_BOOST are not set");
    }
    return basefreq_completed();
}

static int basefreq_file_baf(topology_t *tp, cpufreq_base_t *base)
{
    char buffer[128];
    // This function is incomplete, just for boost.
    if (found_freq) {
        return 0;
    }
    if (filemagic_once_read(FILE_BAF, buffer, sizeof(buffer))) {
        base->frequency = (ullong) atoi(buffer);
        basefreq_found_set(1, 0);
    } else {
        sprintf(reason, "%s", state_msg);
    }
    return basefreq_completed();
}

static int basefreq_file_ntb(topology_t *tp, cpufreq_base_t *base)
{
    char buffer[128];
    // This function is incomplete, just for boost.
    if (found_boost) {
        return 0;
    }
    // intel_pstate (https://wiki.archlinux.org/title/CPU_frequency_scaling)
    if (filemagic_once_read(FILE_NTB, buffer, sizeof(buffer))) {
        base->boost_enabled = atoi(buffer);
        base->boost_enabled = !base->boost_enabled;
        basefreq_found_set(0, 1);
    } else {
        sprintf(reason, "%s", state_msg);
    }
    return basefreq_completed();
}

static int basefreq_file_cpb(topology_t *tp, cpufreq_base_t *base)
{
    char buffer[128];
    // This function is incomplete, just for boost.
    if (found_boost) {
        return 0;
    }
    // non intel_pstate
    if (filemagic_once_read(FILE_BST, buffer, sizeof(buffer))) {
        base->boost_enabled = atoi(buffer);
        basefreq_found_set(0, 1);
    } else if (filemagic_once_read(FILE_CPB, buffer, sizeof(buffer))) {
        base->boost_enabled = atoi(buffer);
        basefreq_found_set(0, 1);
    } else {
        sprintf(reason, "%s (both)", state_msg);
    }
    return basefreq_completed();
}

static ullong file_read_value(int fd)
{
    char buffer[8];
    int i = 0;
    char c;

    while ((read(fd, &c, sizeof(char)) > 0) && ((c >= '0') && (c <= '9'))) {
        buffer[i] = c;
        i++;
    }
    buffer[i] = '\0';
    return (ullong) atoi(buffer);
}

static int basefreq_file_saf(topology_t *tp, cpufreq_base_t *base)
{
    ullong f0, f1;
    int fd;
    // This is a complete function
    if ((fd = open(FILE_SAF, O_RDONLY)) < 0) {
        sprintf(reason, "%s", strerror(errno));
        return 0;
    }
    f0 = file_read_value(fd);
    f1 = file_read_value(fd);
    close(fd);
    // If the difference is 1 KHz, base/nominal is f1
    if (!found_freq)
        base->frequency = ((f0 - f1) == 1000) ? f1 : f0;
    if (!found_boost)
        base->boost_enabled = ((f0 - f1) == 1000) ? 1 : 0;
    return basefreq_found_set(1, 1);
}

static int basefreq_file_cpuinfo(topology_t *tp, cpufreq_base_t *base)
{
    char *line = NULL;
    char *col  = NULL; // column
    char *aux  = NULL;
    ullong fr2 = 0LLU;
    ullong fr1 = 0LLU;
    size_t len = 0;
    FILE *fd   = NULL;

    // This function is incomplete, just for frequency.
    if (found_freq) {
        return 0;
    }
    if ((fd = fopen("/proc/cpuinfo", "r")) == NULL) {
        sprintf(reason, "%s", strerror(errno));
        return 0;
    }
    while (getline(&line, &len, fd) != -1) {
        if (fr1 == 0LLU && strncmp(line, "model name", 10) == 0) {
            if ((col = strchr(line, '@')) != NULL) {
                col++;
                while (*col == ' ')
                    col++;
                aux = col;
                while (*col != 'G')
                    col++;
                *col = '\0';
                // Converted to KHz
                double f = atof(aux);
                fr1      = ((ullong) f) * 1000000LLU;
            }
        }
        // cpu MHz has priority
        if (fr2 == 0LLU && strncmp(line, "cpu MHz", 7) == 0) {
            if ((col = strchr(line, ':')) != NULL) {
                col++;
                col++;
                double m, i, f = atof(col);
                m = modf(f / 100.0, &i);
                // If the decimal part is 0.0
                if (m == 0.0) {
                    fr2 = ((ullong) f) * 1000LLU;
                }
            }
        }
    }
    fclose(fd);
    free(line);

    // Priorities
    if (fr1 != 0LLU) {
        base->frequency = fr1;
        basefreq_found_set(1, 0);
    } else if (fr2 != 0LLU) {
        base->frequency = fr2;
        basefreq_found_set(1, 0);
    }
    return basefreq_completed();
}

static int basefreq_file_smf(topology_t *tp, cpufreq_base_t *base)
{
    char buffer[128];
    // This function is turbo boost sensitive. Is for frequency, but
    // if the frequency is already found, and the found frequency is not
    // under the CMF or SMF frequency, it means that turbo boost
    // could be enabled. We don't considere this method reliable.
    if (filemagic_once_read(FILE_CMF, buffer, sizeof(buffer)) ||
        filemagic_once_read(FILE_SMF, buffer, sizeof(buffer))) {
        if (!found_boost && found_freq) {
            base->boost_enabled = ((ullong) atoi(buffer) > base->frequency);
            basefreq_found_set(0, 1);
        }
        if (!found_freq) {
            base->frequency    = (ullong) atoi(buffer);
            base->not_reliable = 1;
            basefreq_found_set(1, 0);
        }
    } else {
        sprintf(reason, "%s", strerror(errno));
    }
    return basefreq_completed();
}

static int basefreq_spec_valididity_test(cpufreq_base_t *base)
{
    // Finally, if is under 5 GHz and over 1 GHz we assume that
    // the speculation could be valid.
    if (base->frequency <= 5000000 && base->frequency >= 1000000) {
        base->not_reliable = 1;
        return basefreq_found_set(1, 0);
    }
    sprintf(reason, "%llu is not a valid frequency", base->frequency);
    return 0;
}

static int basefreq_spec_tsc(topology_t *tp, cpufreq_base_t *base)
{
    ullong l1, h1, l2, h2;
    double chz;
    // This function is incomplete, just for frequency.
    if (found_freq) {
        return 0;
    }
    // The ARM TSC doesn't work at base frequency. CNTFRQ_EL0 has the
    // TSC (CNTVCT_EL0 or CNTPCT_EL0) frequency. From ARMv8.6 is 1GHz.
    if (tp->vendor == VENDOR_ARM) {
        sprintf(reason, "Can't be speculated by TSC in ARM");
        return 0;
    }
    l1 = h1 = 0LLU;
    l2 = h2 = 0LLU;
#if __ARCH_X86
    asm volatile("rdtsc" : "=a"(l1), "=d"(h1));
#endif
    stress_spin(1000);
#if __ARCH_X86
    asm volatile("rdtsc" : "=a"(l2), "=d"(h2));
#endif
    h1 = h1 << 32 | l1;
    h2 = h2 << 32 | l2;
    // CHz is an invented unit. Is thousands of MHz and is
    // used to round the counting cycles.
    chz = ((double) (h2 - h1)) / 100000000.0;
    // debug("Cycles TSC: %llu, CHz: %0.2lf", h2-h1, chz);
    //  Passing to frequency and checking
    base->frequency = ((ullong) round(chz)) * 100000LU;
    return basefreq_spec_valididity_test(base);
}

static int basefreq_spec_asm(topology_t *tp, cpufreq_base_t *base)
{
    timestamp_t ts1;
    double ns, chz;
    // This function is incomplete, just for frequency.
    if (found_freq) {
        return 0;
    }
// Around 4.0 GHz of ASM instructions. This method is turbo boost
// sensitive, so can not find the correct frequency in these nodes
// whose boost technology is enabled. But is better than nothing.
#define DO_40(X) X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X X
    timestamp_get(&ts1);
#if __ARCH_ARM
    // Getting the timestamp we can approximate the CPU frequency.
    // The ADD, CMP and BNE are not considered because the CPU seems
    // to perform a speculative execution, not consuming even one cycle.
    asm volatile("mov x0, #0;"
                 "mov x1, #10000;"
                 "mov x2, #10000;"
                 "mul x1, x1, x2;"
                 "Loop:" DO_40("mov x8, x8;") // 40 cycles
                 "add x0, x0, #1;"            // 1 cycle
                 "cmp x0, x1;"                // 1 cycle
                 "bne Loop;"                  // 2 cycles
    );
#else
    // On the contrary, MOV and NOP seems to not work well on X86,
    // so we use the traditional ADD instruction.
    asm volatile("mov $0, %eax;"
                 "mov $10000, %ebx;"
                 "imul %ebx, %ebx;"
                 "Loop:"
                 "add $1, %eax;"        // 1 cycle
                 DO_40("add $1, %ecx;") // 40 cycles
                 "cmp %ebx, %eax;"      // 1 cycle
                 "jl Loop;"             // X cycles
    );
#endif
    ns  = (double) timestamp_diffnow(&ts1, TIME_NSECS);
    chz = (4100000000.0 / ns) * 10.0; // Hundreds of MHz
    // debug("Time nS: %0.2lf, CHz: %0.2lf", ns, chz);
    //  Passing to frequency and checking
    base->frequency = ((ullong) round(chz)) * 100000LLU;
    return basefreq_spec_valididity_test(base);
}

static int basefreq_spec_hardcode(topology_t *tp, cpufreq_base_t *base)
{
    // This function is incomplete, just for frequency.
    if (!found_freq) {
        base->not_reliable = 1;       // Also is not reliable
        base->frequency    = 2000000; // 2GHz
    }
    // It the boost is not found yet, we assume is not enabled.
    return basefreq_found_set(1, 0);
}

static void basefreq_init_static(topology_t *tp, cpufreq_base_t *base)
{
    // no_turbo has the priority over the hardware/bios
    if (basefreq_file_ntb(tp, base))
        return;
    if (basefreq_intel_haswell(tp, base))
        return;
    if (basefreq_amd_zen(tp, base))
        return;
    if (basefreq_file_baf(tp, base))
        return;
    if (basefreq_environment(tp, base))
        return;
    if (basefreq_file_cpb(tp, base))
        return;
    if (basefreq_file_saf(tp, base))
        return;
    if (basefreq_file_cpuinfo(tp, base))
        return;
    if (basefreq_spec_tsc(tp, base))
        return;
    if (basefreq_file_smf(tp, base))
        return;
    if (basefreq_spec_asm(tp, base))
        return;
    if (basefreq_spec_hardcode(tp, base))
        return;
}

__attribute__((unused)) static void cpufreq_base_tests(topology_t *tp, int fd)
{
    char *c[2] = {"-", "f"};
    cfb_t bf;
    int n = 0;

    dprintf(fd, "test   frequency  f  boost  f   !rel   name                                !reason\n");
    dprintf(fd, "----   ---------  -  -----  -   ----   ----                                -------\n");
#define PRINTEST(call, string)                                                                                         \
    {                                                                                                                  \
        memset(&bf, 0, sizeof(cfb_t));                                                                                 \
        basefreq_clean();                                                                                              \
        call;                                                                                                          \
        dprintf(fd, "%4d   %9llu  %s  %5d  %s   %4d   %s  %s\n", n++, bf.frequency, c[found_freq], bf.boost_enabled,   \
                c[found_boost], bf.not_reliable, string, reason);                                                      \
    }
    PRINTEST(basefreq_file_ntb(tp, &bf), "no_turbo file                     ");
    PRINTEST(basefreq_intel_haswell(tp, &bf), "intel cpuid                       ");
    PRINTEST(basefreq_amd_zen(tp, &bf), "amd msr                           ");
    PRINTEST(basefreq_file_baf(tp, &bf), "base_frequency file               ");
    PRINTEST(basefreq_environment(tp, &bf), "environment                       ");
    PRINTEST(basefreq_file_cpb(tp, &bf), "cpb/boost files                   ");
    PRINTEST(basefreq_file_saf(tp, &bf), "scaling_available_frequencies file");
    PRINTEST(basefreq_file_cpuinfo(tp, &bf), "proc/cpuinfo                      ");
    PRINTEST(basefreq_spec_tsc(tp, &bf), "spec tsc                          ");
    PRINTEST(basefreq_file_smf(tp, &bf), "cpuinfo_max_freq files            ");
    PRINTEST(basefreq_spec_asm(tp, &bf), "spec asm                          ");
    PRINTEST(basefreq_spec_hardcode(tp, &bf), "hardcode                          ");
}

void cpufreq_base_init(topology_t *tp, cpufreq_base_t *base)
{
    if (tp == NULL) {
        if (!tp_static.initialized) {
            topology_init(&tp_static);
        }
        tp = &tp_static;
    }
    if (bs_static.frequency == 0) {
#if SHOW_DEBUGS
        cpufreq_base_tests(tp, debug_channel);
#endif
        basefreq_init_static(tp, &bs_static);
    }
    memcpy(base, &bs_static, sizeof(cpufreq_base_t));
}

#if TEST
// gcc -DTEST=1 -I ../../ -o test cpufreq_base.c ../../metrics/libmetrics.a ../../common/libcommon.a -lm -lpthread &&
// ./test
static topology_t tp;

int main(int argc, char *argv[])
{
    topology_init(&tp);
    cpufreq_base_tests(&tp, 0);
    return 0;
}
#endif
