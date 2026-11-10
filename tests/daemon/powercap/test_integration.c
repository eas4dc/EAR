/*
 * test_integration.c -- EARD powercap end to end, on fake hardware.
 *
 * Real code: powercap.c, powercap_mgt.c, powercap_status.c, the real plugins
 * cpu_generic.so and gpu.so (dlopened exactly as EARD does), and the real
 * libmgt/libmetrics/libcommon underneath them (intel63 RAPL caps through MSRs,
 * NVML GPU caps, RAPL energy, the monitor thread).
 *
 * Fake: the hardware, from the ear-fake-devices repo --
 *   libfakesys.so  (LD_PRELOAD)     /dev/cpu/N/msr incl. RAPL power-limit
 *                                   read-back, 2 fake sockets (topo_sockets)
 *   libfakenvml.so (HACK_NVML_FILE) per-GPU power limits and utilization
 * the power monitor (fake_powermon.c) and EARGM (eargm_sim.c).
 *
 * The oracle is the hardware itself: after every action the test reads the
 * caps back from the MSRs and from NVML, independently of EAR.
 *
 * run_integration.sh runs this binary once per rig; the rig is described by
 * the environment it sets:
 *   IT_GPUS       number of GPUs NVML reports (0, 2, 4)
 *   IT_GPU_UTIL   comma separated utilization per GPU (what fakenvml serves)
 *   IT_PCDIR      directory with cpu_generic.so and gpu.so
 * Rig constants (must match the generated fake configs): PKG TDP 200 W,
 * DRAM TDP 40 W, 2 sockets, GPU cap range 100..400 W.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>

#include <common/config.h>
#include <common/hardware/topology.h>
#include <common/types/configuration/cluster_conf.h>
#include <daemon/powercap/powercap.h>
#include <daemon/powercap/powercap_mgt.h>
#include <daemon/powercap/powercap_status.h>

#include "eargm_sim.h"
#include "fake_powermon.h"
#include "pc_test.h"

/* Globals EARD provides */
cluster_conf_t my_cluster_conf;
static my_node_conf_t node_conf;
my_node_conf_t *my_node_conf = &node_conf;

extern node_powercap_opt_t my_pc_opt;
extern int *ips;

#define PKG_TDP  200
#define DRAM_TDP 40
#define GPU_MIN  100
#define GPU_MAX  400
#define SOCKETS  2
#define DEF      2000
#define MAX      3000

static int gpus;

/* Highest per-socket cap the hardware takes: the RAPL POWER_INFO maximum in the
 * limits rigs (250/60 W, run_integration.sh), the TDP when POWER_INFO reports none */
static int limits_rig(void);
static int pkg_ceil(void)
{
    return limits_rig() ? 250 : PKG_TDP;
}
static int dram_ceil(void)
{
    return limits_rig() ? 60 : DRAM_TDP;
}
static uint gpu_util[8];
static int socket_cpu[SOCKETS]; /* first CPU of each socket, as intel63 uses */

/* ============================================================ hardware */

static double rapl_power_unit(void)
{
    int fd = open("/dev/cpu/0/msr", O_RDONLY);
    uint64_t v = 0;
    pread(fd, &v, sizeof(v), 0x606);
    close(fd);
    return pow(0.5, (double) (v & 0xf));
}

/* Returns the PL1 cap of a socket in W, or -1 when the cap is disabled */
static int hw_rapl_cap(int socket, off_t reg)
{
    char p[64];
    snprintf(p, sizeof(p), "/dev/cpu/%d/msr", socket_cpu[socket]);
    int fd = open(p, O_RDONLY);
    uint64_t v = 0;
    pread(fd, &v, sizeof(v), reg);
    close(fd);
    if (!(v & (1ULL << 15)))
        return -1;
    return (int) lround((double) (v & 0x7fff) * rapl_power_unit());
}
#define hw_pkg(s)  hw_rapl_cap(s, 0x610)
#define hw_dram(s) hw_rapl_cap(s, 0x618)

typedef int (*nvml_init_t)(void);
typedef int (*nvml_handle_t)(unsigned, void **);
typedef int (*nvml_limit_t)(void *, unsigned *);
static nvml_handle_t nv_handle;
static nvml_limit_t nv_limit;

static void nvml_open(void)
{
    void *lib = dlopen(getenv("HACK_NVML_FILE"), RTLD_NOW | RTLD_GLOBAL);
    if (lib == NULL)
        return;
    ((nvml_init_t) dlsym(lib, "nvmlInit_v2"))();
    nv_handle = (nvml_handle_t) dlsym(lib, "nvmlDeviceGetHandleByIndex_v2");
    nv_limit  = (nvml_limit_t) dlsym(lib, "nvmlDeviceGetPowerManagementLimit");
}

static int hw_gpu(int i)
{
    void *h = NULL;
    unsigned mw = 0;
    if (!nv_handle || nv_handle(i, &h) != 0 || nv_limit(h, &mw) != 0)
        return -1;
    return (int) (mw / 1000);
}

/* NVML maximum power cap of GPU i (W) */
static int hw_gpu_max(int i)
{
    typedef int (*nvml_range_t)(void *, unsigned *, unsigned *);
    static nvml_range_t f;
    void *h        = NULL;
    unsigned mn    = 0, mx = 0;
    if (!f)
        f = (nvml_range_t) dlsym(dlopen(getenv("HACK_NVML_FILE"), RTLD_NOW), "nvmlDeviceGetPowerManagementLimitConstraints");
    if (!f || !nv_handle || nv_handle(i, &h) != 0 || f(h, &mn, &mx) != 0)
        return -1;
    return (int) (mx / 1000);
}

static int hw_total(void)
{
    int t = 0;
    for (int s = 0; s < SOCKETS; s++)
        t += (hw_pkg(s) > 0 ? hw_pkg(s) : PKG_TDP) + (hw_dram(s) > 0 ? hw_dram(s) : DRAM_TDP);
    for (int g = 0; g < gpus; g++)
        t += hw_gpu(g);
    return t;
}

/* Measured CPU power (PKG + DRAM, all sockets) over ms, from the RAPL
 * energy counters, independently of EAR */
static double hw_cpu_power(int ms)
{
    int fd = open("/dev/cpu/0/msr", O_RDONLY);
    uint64_t unit = 0;
    pread(fd, &unit, sizeof(unit), 0x606);
    close(fd);
    double ej = pow(0.5, (double) ((unit >> 8) & 0x1f));
    uint64_t e0[SOCKETS][2], e1[SOCKETS][2];
    for (int pass = 0; pass < 2; pass++) {
        for (int s = 0; s < SOCKETS; s++) {
            char p[64];
            snprintf(p, sizeof(p), "/dev/cpu/%d/msr", socket_cpu[s]);
            int f = open(p, O_RDONLY);
            uint64_t v[2] = {0, 0};
            pread(f, &v[0], 8, 0x611);
            pread(f, &v[1], 8, 0x619);
            close(f);
            for (int k = 0; k < 2; k++)
                (pass ? e1 : e0)[s][k] = v[k] & 0xffffffffULL;
        }
        if (!pass)
            usleep(ms * 1000);
    }
    double j = 0;
    for (int s = 0; s < SOCKETS; s++)
        for (int k = 0; k < 2; k++)
            j += (double) ((e1[s][k] - e0[s][k]) & 0xffffffffULL) * (k ? pow(0.5, 16) : ej);
    return j / (ms / 1000.0);
}

typedef int (*nvml_power_t)(void *, unsigned *);
typedef int (*nvml_clock_t)(void *, int, unsigned *);
static int hw_gpu_power(int i)
{
    static nvml_power_t f;
    if (!f)
        f = (nvml_power_t) dlsym(dlopen(getenv("HACK_NVML_FILE"), RTLD_NOW), "nvmlDeviceGetPowerUsage");
    void *h = NULL;
    unsigned mw = 0;
    if (!f || !nv_handle || nv_handle(i, &h) != 0 || f(h, &mw) != 0)
        return -1;
    return (int) (mw / 1000);
}
static int hw_gpu_clock(int i)
{
    static nvml_clock_t f;
    if (!f)
        f = (nvml_clock_t) dlsym(dlopen(getenv("HACK_NVML_FILE"), RTLD_NOW), "nvmlDeviceGetClockInfo");
    void *h = NULL;
    unsigned mhz = 0;
    if (!f || !nv_handle || nv_handle(i, &h) != 0 || f(h, 1 /* SM */, &mhz) != 0)
        return -1;
    return (int) mhz;
}

static void print_hw(const char *when)
{
    printf("        hw %-22s PKG %4d %4d  DRAM %3d %3d  GPU", when, hw_pkg(0), hw_pkg(1), hw_dram(0), hw_dram(1));
    for (int g = 0; g < gpus; g++)
        printf(" %4d", hw_gpu(g));
    printf("   (total %d)\n", hw_total());
}

/* =============================================================== oracle */

/* CPU domain (PKG + DRAM) share of a node limit, from the TDPs */
static double cpu_share(void)
{
    double pk = (PKG_TDP + DRAM_TDP) * SOCKETS, g = (double) GPU_MAX * gpus;
    return pk / (pk + g + 0.1 * pk);
}
static double gpu_share(void)
{
    double pk = (PKG_TDP + DRAM_TDP) * SOCKETS, g = (double) GPU_MAX * gpus;
    return g / (pk + g + 0.1 * pk);
}

/* =============================================================== setup */

static const char *cpu_plugin(void)
{
    return getenv("IT_CPU_PLUGIN") ? getenv("IT_CPU_PLUGIN") : "cpu_generic";
}

static const char *gpu_plugin(void)
{
    return getenv("IT_GPU_PLUGIN") ? getenv("IT_GPU_PLUGIN") : "gpu";
}

static int dvfs_rig(void)
{
    return !strcmp(cpu_plugin(), "dvfs");
}

static void it_setup_noreset(uint def);
static void it_setup(uint def)
{
    fake_powermon_reset();
    fake_pc_def = def;
    fake_pc_max = MAX;
    it_setup_noreset(def);
}

static void it_setup_noreset(uint def)
{
    memset(&node_conf, 0, sizeof(node_conf));
    static policy_conf_t pol;
    pol.policy              = 0;
    pol.p_state             = 1;
    node_conf.num_policies  = 1;
    node_conf.policies      = &pol;
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.so", getenv("IT_PCDIR"), cpu_plugin());
    setenv("EAR_POWERCAP_POLICY_CPU", path, 1);
    snprintf(path, sizeof(path), "%s/%s.so", getenv("IT_PCDIR"), gpu_plugin());
    setenv("EAR_POWERCAP_POLICY_GPU", path, 1);
    nvml_open();
    CHECK_EQ(powercap_init(), EAR_SUCCESS, "powercap_init with %s.so%s%s%s", cpu_plugin(), gpus ? " + " : "",
             gpus ? gpu_plugin() : "", gpus ? ".so" : "");
}

static void settle(void)
{
    /* util detection (500 ms, works every 2nd tick) + the GPU plugin thread (1 s burst) */
    usleep(3200000);
}

/* Waits until the fake GPUs are loaded (they start idle, see run_integration.sh) */
static void wait_gpu_load(void)
{
    int idle = getenv("IT_IDLE_S") ? atoi(getenv("IT_IDLE_S")) : 0;
    if (!getenv("IT_BUSY_AT_START") || atoi(getenv("IT_BUSY_AT_START")) == 0)
        sleep(idle);
    settle();
}

static int busy_at_start(void)
{
    return getenv("IT_BUSY_AT_START") && atoi(getenv("IT_BUSY_AT_START"));
}

/* ================================================================ tests */

TEST(it_topology_seen_by_ear)
{
    topology_t t;
    topology_init(&t);
    CHECK_EQ(t.socket_count, SOCKETS, "EAR sees 2 sockets (fakesys topo_sockets)");
}

TEST(it_initial_caps)
{
    it_setup(DEF);
    print_hw("after init");
    int cpu = (int) (DEF * cpu_share());
    int pk  = ear_min((int) (cpu * (double) PKG_TDP / (PKG_TDP + DRAM_TDP) / SOCKETS), pkg_ceil());
    int dr  = ear_min((int) (cpu * (double) DRAM_TDP / (PKG_TDP + DRAM_TDP) / SOCKETS), dram_ceil());
    for (int s = 0; s < SOCKETS; s++) {
        CHECK(abs(hw_pkg(s) - pk) <= 1, "socket %d PKG cap %d W (expected %d)", s, hw_pkg(s), pk);
        CHECK(abs(hw_dram(s) - dr) <= 1, "socket %d DRAM cap %d W (expected %d)", s, hw_dram(s), dr);
    }
    CHECK(hw_total() <= DEF, "all device caps add up to <= the node limit (%d <= %d)", hw_total(), DEF);
    if (gpus) {
        for (int g = 0; g < gpus; g++)
            CHECK(hw_gpu(g) >= GPU_MIN && hw_gpu(g) <= GPU_MAX, "GPU%d cap %d within its range", g, hw_gpu(g));
    }
}

TEST(it_verify_after_init)
{
    it_setup(DEF);
    state_t r = powercap_verify_all_devices();
    /* Plugins store their device values while powercap_init applies the first cap */
    CHECK(r == EAR_SUCCESS, "stored device caps match the hardware right after init (verify=%d)", r);
    CHECK_EQ(powercap_get_stored_device_value(DOMAIN_CPU, 0), (uint32_t) hw_pkg(0),
             "storage holds socket 0's %d W cap", hw_pkg(0));
    /* DRAM is capped by cpu_generic.so inside the CPU domain and read back from RAPL */
    CHECK(powercap_verify_device_values(DOMAIN_DRAM) != EAR_ERROR,
          "DRAM caps (%d/%d W on the hardware) can be verified", hw_dram(0), hw_dram(1));
}

TEST(it_gpu_split_follows_load)
{
    if (!gpus) {
        CHECK(1, "no GPUs in this rig");
        return;
    }
    it_setup(DEF);
    fake_ctx_start(1, 2400000, 1410000, gpus);
    powercap_idle_to_run();
    powercap_new_job();
    wait_gpu_load();
    print_hw("after util detection");
    int gshare = (int) (DEF * gpu_share());
    int busy_above_min = 1, idle_at_min = 1, equal = 1;
    for (int g = 0; g < gpus; g++) {
        if (gpu_util[g] == 0 && hw_gpu(g) != GPU_MIN)
            idle_at_min = 0;
        if (gpu_util[g] > 0 && hw_gpu(g) <= GPU_MIN)
            busy_above_min = 0;
        if (gpu_util[g] == gpu_util[0] && hw_gpu(g) != hw_gpu(0))
            equal = 0;
    }
    CHECK(idle_at_min, "idle GPUs sit at their minimum");
    /* the GPU share (%d W) is well above gpus x min, so every busy GPU must get more than its minimum */
    CHECK(busy_above_min && equal, "busy GPUs get more than their minimum, equal loads equal caps (util %u,%u,%u,%u)",
          gpu_util[0], gpu_util[1], gpu_util[2], gpu_util[3]);
    int gsum = 0;
    for (int g = 0; g < gpus; g++)
        gsum += hw_gpu(g);
    CHECK(gsum <= gshare || gsum <= gpus * GPU_MIN, "GPU caps within the GPU domain share (%d <= %d)", gsum, gshare);
    int cpu_init = (int) (DEF * cpu_share() * PKG_TDP / (PKG_TDP + DRAM_TDP) / SOCKETS);
    CHECK(hw_pkg(0) >= cpu_init / 4, "with busy GPUs the sockets keep at least a quarter of their share (%d of %d W)",
          hw_pkg(0), cpu_init);
    CHECK(hw_total() <= DEF || gsum <= gpus * GPU_MIN, "node total within the limit (%d)", hw_total());
}

TEST(it_soft_powercap_sequence)
{
    it_setup(DEF);
    powercap_idle_to_run();
    int pk2000 = hw_pkg(0);
    eargm_set_powerlimit(1200);
    print_hw("soft cap 1200");
    CHECK(hw_total() <= 1200 || !gpus, "caps shrink under a 1200 W soft cap (%d)", hw_total());
    int pk1200 = hw_pkg(0);
    /* the 1200 W share per socket can still be above the hardware maximum (CPU-only): then it stays there */
    if ((int) (1200 * cpu_share() * PKG_TDP / (PKG_TDP + DRAM_TDP) / SOCKETS) < pkg_ceil() || gpus)
        CHECK(pk1200 > 0 && pk1200 < pk2000, "socket 0 cap lowered (%d -> %d W)", pk2000, pk1200);
    else
        CHECK(pk1200 == pk2000 && pk1200 <= pkg_ceil(), "socket 0 cap held at the hardware maximum (%d -> %d W)",
              pk2000, pk1200);

    eargm_set_powerlimit(POWER_CAP_UNLIMITED);
    print_hw("unlimited");
    /* 'unlimited' restores the registers found at start: PL1 = TDP here */
    CHECK((hw_pkg(0) == -1 || hw_pkg(0) == PKG_TDP) && (hw_pkg(1) == -1 || hw_pkg(1) == PKG_TDP),
          "unlimited: PKG caps back to their power-on value (%d %d)", hw_pkg(0), hw_pkg(1));
    for (int g = 0; g < gpus; g++)
        CHECK_EQ(hw_gpu(g), GPU_MAX, "unlimited: GPU%d back to its max", g);

    eargm_set_powerlimit(1200);
    print_hw("soft cap 1200 again");
    CHECK(hw_pkg(0) == pk1200, "1200 W again after unlimited re-caps the sockets (PKG0 %d, want %d)",
          hw_pkg(0), pk1200);
    for (int g = 0; g < gpus; g++)
        CHECK(hw_gpu(g) < GPU_MAX, "GPU%d re-capped (%d W)", g, hw_gpu(g));
}

TEST(it_limit_clamped_to_max)
{
    it_setup(DEF);
    eargm_set_powerlimit(10000);
    CHECK_EQ(my_pc_opt.current_pc, MAX, "a 10 kW request is clamped to the 3000 W max powercap");
    CHECK(hw_total() <= MAX, "hardware within the max powercap (%d)", hw_total());
}

TEST(it_tiny_limit)
{
    it_setup(DEF);
    eargm_set_powerlimit(300);
    print_hw("limit 300");
    for (int g = 0; g < gpus; g++)
        CHECK_EQ(hw_gpu(g), GPU_MIN, "GPU%d held at its minimum (NVML refuses less)", g);
    CHECK(hw_pkg(0) > 0, "sockets capped low but not disabled (%d W)", hw_pkg(0));
}

TEST(it_manual_per_device)
{
    it_setup(DEF);
    int32_t sock[2] = {150, 120};
    powercap_process_message("set", NULL, "cpu", 2, sock);
    CHECK(hw_pkg(0) == 150 && hw_pkg(1) == 120, "manual cpu 150/120 on the sockets (%d/%d)", hw_pkg(0), hw_pkg(1));
    if (gpus >= 4) {
        int32_t g[4] = {300, 250, 200, 150};
        powercap_process_message("set", NULL, "gpu", 4, g);
        CHECK(hw_gpu(0) == 300 && hw_gpu(1) == 250 && hw_gpu(2) == 200 && hw_gpu(3) == 150,
              "manual per-GPU caps on NVML (%d %d %d %d)", hw_gpu(0), hw_gpu(1), hw_gpu(2), hw_gpu(3));
        settle();
        CHECK(hw_gpu(0) == 300 && hw_gpu(3) == 150, "manual caps survive the plugin threads (%d .. %d)", hw_gpu(0),
              hw_gpu(3));

        int32_t below[4] = {50, 250, 200, 150};
        powercap_process_message("set", NULL, "gpu", 4, below);
        CHECK_EQ(hw_gpu(0), 100, "a 50 W cap (below the NVML min) is clamped to the 100 W minimum");
        CHECK_EQ(powercap_verify_device_values(DOMAIN_GPU), EAR_SUCCESS, "... and stored as programmed");

        int32_t dev[8] = {140, 130, 30, 25, 310, 320, 330, 340};
        powercap_process_message("set", NULL, "device", 8, dev);
        CHECK(hw_pkg(0) == 140 && hw_pkg(1) == 130, "device message: sockets (%d %d)", hw_pkg(0), hw_pkg(1));
        CHECK(hw_gpu(0) == 310 && hw_gpu(3) == 340, "device message: GPUs (%d .. %d)", hw_gpu(0), hw_gpu(3));
        CHECK(hw_dram(0) == 30 && hw_dram(1) == 25, "device message: DRAM caps applied (%d %d)", hw_dram(0),
              hw_dram(1));
    }
    powercap_process_message("deactivate", "manual", NULL, 0, NULL);
}

TEST(it_eargm_cycles_keep_node_under_cap)
{
    it_setup(DEF);
    fake_ctx_start(1, 2400000, 1410000, gpus);
    powercap_idle_to_run();
    powercap_new_job();
    if (gpus)
        wait_gpu_load();
    int32_t me[1] = {0x0a000001};
    int worst     = 0;
    for (int t1 = 0; t1 < 12; t1++) {
        /* power monitor sample: alternate over / under the limit */
        dom_power_t p = {.platform = (t1 % 3 == 0) ? 2300 : 1600, .cpu = 350, .dram = 60, .gpu = 1100};
        fake_node_power = p.platform;
        powercap_set_power_per_domain(&p, 1, 2400000);
        eargm_reply_t r;
        eargm_get_status(&r, 1);
        int32_t extra[1] = {(t1 % 4 == 1) ? 40 : (t1 % 4 == 3 ? -40 : 0)};
        if (r.st.num_greedy)
            eargm_send_opt(1, me, extra, 70);
        else
            eargm_send_opt(0, NULL, NULL, 70);
        eargm_reply_free(&r);
        usleep(300000);
        int over = hw_total() - (int) my_pc_opt.current_pc;
        if (over > worst)
            worst = over;
    }
    print_hw("after 12 EARGM periods");
    CHECK(worst <= 0 || (gpus && hw_total() <= gpus * GPU_MIN + 2 * (PKG_TDP + DRAM_TDP)),
          "over 12 periods the device caps never add up to more than the node cap (worst excess %d W)", worst);
    powercap_end_job();
    powercap_run_to_idle();
    print_hw("back to idle");
    CHECK(hw_total() <= DEF, "idle caps within the default (%d)", hw_total());
}

/* ================================================ DVFS plugins (defaults) */

TEST(it_dvfs_holds_cpu_limit)
{
    /* Node limit chosen so the CPU domain gets ~227 W: well under the 350 W
     * the sockets draw at 3.0 GHz (2 x 150 W PKG + 2 x 25 W DRAM) and above
     * the 150 W they draw at 1.0 GHz, so dvfs.so must find a clock in between */
    uint node = (uint) (227.0 / cpu_share());
    it_setup(node);
    fake_ctx_start(1, 3000000, 1980000, gpus);
    powercap_idle_to_run();
    powercap_new_job();
    double limit = node * cpu_share();
    double p0    = hw_cpu_power(500);
    printf("        measured CPU power %.0f W before the loop reacts (CPU limit %.0f W)\n", p0, limit);
    sleep(8);
    double p = hw_cpu_power(2000);
    /* The CPU limit moves: pmgt lends it spare power from idle GPUs. Compare
     * with the limit dvfs.so is holding now (dvfs.so and gpu_dvfs.so answer
     * get_powercap_value with one value, so this read is safe for them). */
    uint32_t dom[NUM_DOMAINS] = {0};
    pmgt_get_powercap_value(NULL, 0, dom);
    double now = dom[DOMAIN_CPU];
    printf("        measured CPU power %.0f W after 8 s; CPU domain limit now %.0f W (started at %.0f W)\n", p, now,
           limit);
    CHECK(p <= now * 1.05, "dvfs.so keeps PKG + DRAM under its current CPU limit (%.0f <= %.0f W)", p, now);
    CHECK(p >= ear_min(now, 350) * 0.70, "without throttling far below it (%.0f W)", p);

    /* limit raised: frequencies come back to the 3.0 GHz request */
    eargm_set_powerlimit(MAX);
    sleep(8);
    p = hw_cpu_power(2000);
    CHECK(p >= 330, "limit raised: CPUs back at the requested 3.0 GHz (%.0f W)", p);
}

TEST(it_gpu_dvfs_holds_gpu_budget)
{
    if (!gpus) {
        CHECK(1, "no GPUs in this rig");
        return;
    }
    /* 1500 W node: ~282 W per GPU to start with; pmgt then moves power between the CPU
     * and the GPUs, so each sample is judged against the GPU domain's current budget.
     * Busy GPUs draw 50 + 0.2 W/MHz, 446 W at 1980 MHz. */
    it_setup(1500);
    fake_ctx_start(1, 3000000, 1980000, gpus);
    powercap_idle_to_run();
    powercap_new_job();
    wait_gpu_load();
    double budget = 0;
    int over = 0, samples = 0, cmin = 99999, cmax = 0;
    for (int t = 0; t < 30; t++) {
        usleep(500000);
        if (t < 10)
            continue; /* first 5 s: the controller converges from 1980 MHz */
        uint32_t now[NUM_DOMAINS] = {0};
        pmgt_get_powercap_value(NULL, 0, now);
        budget = (double) now[DOMAIN_GPU] / gpus;
        for (int g = 0; g < gpus; g++) {
            int w = hw_gpu_power(g), c = hw_gpu_clock(g);
            over += w > budget + 5;
            samples++;
            if (c < cmin)
                cmin = c;
            if (c > cmax)
                cmax = c;
        }
    }
    printf("        GPU clocks seen over 10 s after settling: %d..%d MHz; %d/%d samples over the per-GPU budget (last %.0f W)\n", cmin, cmax, over,
           samples, budget);
    uint32_t dom[NUM_DOMAINS] = {0};
    pmgt_get_powercap_value(NULL, 0, dom);
    CHECK(dom[DOMAIN_CPU] + dom[DOMAIN_GPU] <= 1500, "CPU + GPU domain limits within the 1500 W node (%u + %u W)",
          dom[DOMAIN_CPU], dom[DOMAIN_GPU]);
    CHECK(over <= samples / 10 && cmax - cmin <= 200,
          "gpu_dvfs.so settles under the per-GPU budget (%d/%d samples over, clock range %d..%d MHz)", over, samples,
          cmin, cmax);
}

/* ========================== cpu_generic.so closed loop (RAPL enforced) */

static int enforce_rig(void)
{
    return getenv("IT_ENFORCE") && atoi(getenv("IT_ENFORCE"));
}

/* Average effective frequency (scaling_cur_freq) of socket s's CPUs, kHz */
static long hw_cpu_freq(int s)
{
    long n = sysconf(_SC_NPROCESSORS_CONF), sum = 0, cnt = 0;
    for (long c = 0; c < n; c++) {
        char p[96], b[32] = "";
        snprintf(p, sizeof(p), "/sys/devices/system/cpu/cpu%ld/cpufreq/scaling_cur_freq", c);
        int fd = open(p, O_RDONLY);
        if (fd < 0)
            continue;
        if (read(fd, b, sizeof(b) - 1) > 0 && c >= (s ? socket_cpu[1] : 0) && (s || c < socket_cpu[1])) {
            sum += atol(b);
            cnt++;
        }
        close(fd);
    }
    return cnt ? sum / cnt : 0;
}

TEST(it_generic_throttled_node_asks_for_power)
{
    /* CPU domain limit ~227 W; at the requested 3.0 GHz the sockets want
     * 2 x 150 W PKG + 2 x 25 W DRAM = 350 W, so RAPL holds them at the caps
     * and the CPUs run slower than the application asked */
    uint node = (uint) (227.0 / cpu_share());
    it_setup(node);
    fake_ctx_start(1, 3000000, 1980000, gpus);
    powercap_idle_to_run();
    powercap_new_job();
    usleep(1500000);
    double p = hw_cpu_power(2000);
    long f   = hw_cpu_freq(0);
    printf("        measured CPU power %.0f W (CPU limit %.0f W); socket 0 runs at %ld MHz, app asked 3000 MHz\n", p,
           node * cpu_share(), f / 1000);
    if (gpus) {
        /* the GPUs start idle: pmgt hands their power to the throttled CPU, so check the
         * CPU share is never drained below its floor (a quarter of it, no RAPL minimum here) */
        int cpu_floor = (int) (node * cpu_share() * PKG_TDP / (PKG_TDP + DRAM_TDP) / SOCKETS / 4);
        int lowest    = hw_pkg(0);
        for (int w = 0; w < 10; w++) {
            usleep(1000000);
            if (hw_pkg(0) < lowest)
                lowest = hw_pkg(0);
        }
        CHECK(lowest >= cpu_floor - 1, "the sockets are never drained below their floor (lowest %d W, floor %d W)",
              lowest, cpu_floor);
        return;
    }
    CHECK(p <= node * cpu_share() + 2, "RAPL holds PKG + DRAM at the caps cpu_generic programmed (%.0f W)", p);
    CHECK(f < 3000000, "the sockets are throttled below the requested 3.0 GHz (%ld MHz)", f / 1000);

    fake_node_power = (uint) p + 30;
    eargm_reply_t r;
    eargm_get_status(&r, 0);
    /* the cpufreq driver reports the REQUESTED frequency (scaling_setspeed /
     * scaling_max_freq): throttling must be judged from APERF/MPERF */
    CHECK(r.pmgt.status == PC_STATUS_GREEDY && r.pmgt.requested > 0,
          "a node RAPL-throttled below its requested frequency asks EARGM for power (status %u, %u W)", r.pmgt.status,
          r.pmgt.requested);
    eargm_reply_free(&r);
}

TEST(it_generic_release_matches_headroom)
{
    /* Large CPU limit, 350 W demand: cpu_generic offers half the headroom */
    uint node = (uint) (700.0 / cpu_share());
    it_setup(node);
    fake_ctx_start(1, 3000000, 1980000, gpus);
    powercap_idle_to_run();
    powercap_new_job();
    double lim = node * cpu_share();
    usleep(1500000);
    eargm_reply_t r;
    eargm_get_status(&r, 0); /* first call primes cpu_generic's energy baseline */
    eargm_reply_free(&r);
    sleep(2);
    double p = hw_cpu_power(1000);
    eargm_get_status(&r, 0);
    /* half the headroom under the CPU cap, which the hardware maximum bounds; a node limit
     * above what the devices can take is released whole (#788) */
    double hw_cpu = SOCKETS * (pkg_ceil() + dram_ceil());
    double want   = ear_max((ear_min(lim, hw_cpu) - p) * 0.5, (double) node - hw_cpu);
    printf("        CPU limit %.0f W, measured %.0f W: expect RELEASE of ~%.0f W, got status %u tbr %u\n", lim, p, want,
           r.pmgt.status, r.pmgt.tbr);
    if (gpus == 0) {
        CHECK_EQ(r.pmgt.status, PC_STATUS_RELEASE, "CPU-only node with 350 of %.0f W used: RELEASE", lim);
        CHECK(fabs((double) r.pmgt.tbr - want) <= 0.1 * want, "offers half the measured headroom (%u vs %.0f W)",
              r.pmgt.tbr, want);
    }
    eargm_reply_free(&r);

    /* EARGM takes it: the node cap drops, RAPL follows, power stays under */
    uint before = my_pc_opt.current_pc;
    eargm_get_status(&r, 1);
    eargm_reply_free(&r);
    usleep(3000000); /* RAPL holds the average over its time window: let the new caps settle */
    p = hw_cpu_power(1000);
    printf("        after the release: node cap %u -> %u W, CPU power %.0f W\n", before, my_pc_opt.current_pc, p);
    CHECK(my_pc_opt.current_pc <= before, "node cap lowered by the release");
    int cpu_caps = hw_pkg(0) + hw_pkg(1) + hw_dram(0) + hw_dram(1);
    /* with GPUs the CPU may borrow their idle power: hold it to the caps it was given */
    double cpu_lim = gpus ? cpu_caps : my_pc_opt.current_pc * cpu_share();
    CHECK(p <= cpu_lim * 1.02 + 2, "measured CPU power within its caps + 2%% (%.0f W, caps %.0f W)", p, cpu_lim);
    if (gpus)
        CHECK(p >= 340, "the CPU keeps the 350 W it was using when idle GPUs release theirs (%.0f W)", p);
}

/* Measured node power: CPU from RAPL + every GPU from NVML */
static double hw_node_power(void)
{
    double p = hw_cpu_power(500);
    for (int g = 0; g < gpus; g++)
        p += hw_gpu_power(g);
    return p;
}

TEST(it_node_stays_under_cap_through_eargm)
{
    it_setup(DEF);
    fake_ctx_start(1, 3000000, 1410000, gpus);
    powercap_idle_to_run();
    powercap_new_job();
    if (gpus)
        wait_gpu_load();
    /* periods 0-5: EARGM in soft mode (SET_POWER, status reads only); periods 6-11: restarted
     * in hard mode (status with release, greedy grants), which drops the soft limit */
    uint32_t caps[] = {1600, 1200, 900};
    int32_t me[1]   = {0x0a000001};
    double worst    = -1e9;
    double worst_all = -1e9;
    int caps_over   = -100000;
    uint prev_cap   = UINT32_MAX;
    for (int t1 = 0; t1 < 12; t1++) {
        int hard = t1 >= 6;
        if (!hard && t1 % 2 == 0)
            eargm_set_powerlimit(caps[t1 / 2]);
        eargm_reply_t r;
        fake_node_power = (uint) hw_node_power();
        eargm_get_status(&r, hard);
        if (hard) {
            int32_t extra[1] = {r.st.num_greedy ? 30 : 0};
            eargm_send_opt(r.st.num_greedy ? 1 : 0, me, extra, 70);
        }
        eargm_reply_free(&r);
        if (!hard)
            CHECK_EQ(my_pc_opt.current_pc, caps[t1 / 2], "soft mode, period %d: the node follows the soft limit", t1);
        else if (t1 == 6)
            CHECK(my_pc_opt.current_pc == my_pc_opt.last_t1_allocated,
                  "hard mode again: the soft limit is dropped, cap = allocation (%u W)", my_pc_opt.current_pc);
        usleep(1200000);
        double p     = hw_node_power();
        double floor = gpus * 100.0; /* GPUs cannot go under their minimum cap */
        /* A loaded GPU at its lowest clock can't draw less, whatever the cap: that
         * overshoot is the node working minimum (#788), not EARD failing its cap */
        int clk_min = getenv("IT_GPU_CLK_MIN") ? atoi(getenv("IT_GPU_CLK_MIN")) : 0;
        double pinned = 0;
        for (int g = 0; g < gpus; g++)
            if (hw_gpu_clock(g) <= clk_min && hw_gpu_power(g) > hw_gpu(g))
                pinned += hw_gpu_power(g) - hw_gpu(g);
        double ex = p - pinned - ear_max((double) my_pc_opt.current_pc, floor + 100);
        printf("        period %2d: node cap %4u W, measured CPU+GPU %4.0f W\n", t1, my_pc_opt.current_pc, p);
        if (p > my_pc_opt.current_pc)
            print_hw("  over the cap:");
        /* what EARD programs: device caps must add up to the node cap at most */
        int programmed = 0;
        for (int s = 0; s < SOCKETS; s++)
            programmed += hw_pkg(s) + hw_dram(s);
        for (int g = 0; g < gpus; g++)
            programmed += hw_gpu(g);
        int cover = programmed - (int) ear_max((double) my_pc_opt.current_pc, floor + 100);
        if (cover > caps_over)
            caps_over = cover;
        /* measured: right after a cap change the window still holds power drawn
         * under the previous cap and the fake GPUs ramp, so judge settled periods
         * with a 2% sampling margin */
        if (ex > worst_all)
            worst_all = ex;
        if (my_pc_opt.current_pc == prev_cap && ex - 0.02 * my_pc_opt.current_pc > worst)
            worst = ex - 0.02 * my_pc_opt.current_pc;
        prev_cap = my_pc_opt.current_pc;
    }
    if (gpus == 0)
        CHECK(worst_all <= 5, "measured CPU power never exceeds the node cap across 12 EARGM periods (worst %+.0f W)",
              worst_all);
    else {
        CHECK(caps_over <= 2, "programmed CPU + GPU caps never add up to more than the node cap (worst %+d W)",
              caps_over);
        CHECK(worst <= 5,
              "measured CPU + GPU power stays under the node cap in settled periods (worst %+.0f W over cap + 2%%)",
              worst);
    }
}

/* ===================================== caps outside the hardware limits */
/*
 * Rig limits-*: RAPL reports PKG 60..250 W and DRAM 10..60 W per socket in
 * POWER_INFO; NVML reports 100..400 W per GPU. The firmware holds any limit
 * inside those ranges. EARD clamps every cap to them, never applies a node
 * cap under the node working minimum and offers back what the devices can
 * never use (#788).
 */
#define PKG_MIN  60
#define PKG_MAX  250
#define DRAM_MIN 10
#define GPU_MINW 100
#define GPU_MAXW 400

static int limits_rig(void)
{
    return getenv("IT_LIMITS") && atoi(getenv("IT_LIMITS"));
}

/* Node minimum: what the devices draw at their lowest limits */
static double node_min_w(void)
{
    return SOCKETS * (PKG_MIN + DRAM_MIN) + gpus * GPU_MINW;
}

TEST(it_cap_below_hardware_minimum)
{
    it_setup(DEF);
    fake_ctx_start(1, 3000000, 1410000, gpus);
    powercap_idle_to_run();
    powercap_new_job();
    if (gpus)
        wait_gpu_load();
    uint cap = (uint) (node_min_w() * 0.55); /* well under what the node can go down to */
    eargm_set_powerlimit(cap);
    usleep(2500000);
    print_hw("cap under the minimum");
    double p = hw_cpu_power(1000);
    for (int g = 0; g < gpus; g++)
        p += hw_gpu_power(g);
    printf("        node cap %u W, hardware minimum %.0f W, measured %.0f W\n", cap, node_min_w(), p);
    CHECK(p > cap, "the node cannot meet a cap under its hardware minimum (measured %.0f W > %u W)", p, cap);
    CHECK(p <= node_min_w() + 20, "the firmware holds every device at its minimum (%.0f W)", p);

    int below = 0;
    for (int s = 0; s < SOCKETS; s++)
        below += hw_pkg(s) > 0 && hw_pkg(s) < PKG_MIN;
    CHECK(below == 0, "PKG caps are clamped to the RAPL minimum (%d W / %d W programmed, min %d W)", hw_pkg(0),
          hw_pkg(1), PKG_MIN);

    eargm_reply_t r;
    fake_node_power = (uint) p;
    eargm_get_status(&r, 0);
    printf("        status to EARGM: %u, allocation reported %u W, cap %u W, requested %u W\n", r.pmgt.status,
           r.st.total_powercap, my_pc_opt.current_pc, r.st.requested);
    /* The soft limit is the cap; the allocation (the default) is not what the node runs at */
    CHECK(my_pc_opt.current_pc >= (uint) (node_min_w() * 0.95),
          "the node reports the cap it can hold, its real minimum, not the %u W it cannot meet", my_pc_opt.current_pc);
    eargm_reply_free(&r);
}

TEST(it_cap_above_hardware_maximum)
{
    it_setup(DEF);
    fake_ctx_start(1, 3000000, 1410000, gpus);
    powercap_idle_to_run();
    powercap_new_job();
    if (gpus)
        wait_gpu_load();
    eargm_set_powerlimit(MAX); /* 3000 W: more than every device can take */
    usleep(300000); /* read the caps before PC-17 moves power between domains */
    print_hw("cap above the maximum");
    int above = 0;
    for (int s = 0; s < SOCKETS; s++)
        above += hw_pkg(s) > PKG_MAX;
    CHECK(above == 0, "PKG caps are clamped to the RAPL maximum (%d / %d W programmed, max %d W)", hw_pkg(0),
          hw_pkg(1), PKG_MAX);
    int gpu_above = 0;
    for (int g = 0; g < gpus; g++)
        gpu_above += hw_gpu(g) > GPU_MAXW;
    CHECK_EQ(gpu_above, 0, "GPU caps never above the NVML maximum (gpu.so clamps its split)");

    /* the node can use at most SOCKETS*(PKG_MAX+60) + gpus*GPU_MAXW; the rest
     * of the 3000 W should go back to EARGM */
    double usable = SOCKETS * (PKG_MAX + 60) + gpus * GPU_MAXW;
    eargm_reply_t r;
    double p = hw_cpu_power(1000);
    for (int g = 0; g < gpus; g++)
        p += hw_gpu_power(g);
    fake_node_power = (uint) p;
    eargm_get_status(&r, 0);
    printf("        node cap %u W, usable at most %.0f W, measured %.0f W; status %u tbr %u\n", my_pc_opt.current_pc,
           usable, p, r.pmgt.status, r.pmgt.tbr);
    CHECK(r.pmgt.status == PC_STATUS_RELEASE && r.pmgt.tbr >= (uint) (MAX - usable) * 0.9,
          "the %.0f W the devices can never use are offered back to EARGM (status %u, tbr %u)", MAX - usable,
          r.pmgt.status, r.pmgt.tbr);
    eargm_reply_free(&r);
}

TEST(it_device_caps_outside_hardware_range)
{
    it_setup(DEF);
    int32_t low[2] = {20, 20}, high[2] = {400, 400};
    powercap_process_message("set", NULL, "cpu", 2, low);
    usleep(300000);
    double p = hw_cpu_power(1000);
    printf("        per-socket 20 W (min %d): PL1 %d/%d W programmed, measured CPU %.0f W\n", PKG_MIN, hw_pkg(0),
           hw_pkg(1), p);
    CHECK(hw_pkg(0) >= PKG_MIN, "a per-socket cap under the RAPL minimum is refused or clamped (programmed %d W)",
          hw_pkg(0));
    powercap_process_message("set", NULL, "cpu", 2, high);
    usleep(300000);
    CHECK(hw_pkg(0) <= PKG_MAX, "a per-socket cap above the RAPL maximum is refused or clamped (programmed %d W)",
          hw_pkg(0));
    if (gpus >= 4) {
        int32_t g[4] = {50, 500, 200, 200};
        int before0 = hw_gpu(0), before1 = hw_gpu(1);
        powercap_process_message("set", NULL, "gpu", 4, g);
        CHECK(hw_gpu(0) == GPU_MINW && hw_gpu(1) == GPU_MAXW,
              "50 W and 500 W (outside 100..400) are clamped to the NVML range: GPU0/1 at %d/%d W (were %d/%d)",
              hw_gpu(0), hw_gpu(1), before0, before1);
        CHECK(powercap_verify_device_values(DOMAIN_GPU) == EAR_SUCCESS,
              "the clamped per-GPU caps are stored as programmed (verify=%d)",
              powercap_verify_device_values(DOMAIN_GPU));
    }
    powercap_process_message("deactivate", "manual", NULL, 0, NULL);
}

/* ear.conf powercap=-1: the EARGM's power is split among its nodes. Nodes under the
 * first EARGM in the file (index 0) were treated as "invalid EARGM" -> UNLIMITED */
TEST(it_conf_auto_powercap_first_eargm)
{
    char path[] = "/tmp/pc_it_ear_conf_XXXXXX";
    int fd      = mkstemp(path);
    dprintf(fd, "EARGMId=1 energy=0 power=1000 node=nodeA port=50100\n"
                "EARGMId=2 energy=0 power=3000 node=nodeB port=50101\n"
                "Tag=all default=yes powercap=-1\n"
                "Island=0 Nodes=pcnode[1-4] EARGMID=1\n"
                "Island=0 Nodes=pcother[1-2] EARGMID=2\n");
    close(fd);
    cluster_conf_t conf;
    memset(&conf, 0, sizeof(conf));
    int r = read_cluster_conf(path, &conf);
    unlink(path);
    /* the file lacks sections read_cluster_conf wants (DB...) but islands, tags and EARGMs are parsed */
    printf("        read_cluster_conf on a minimal file: %d\n", r);
    my_node_conf_t *first = get_my_node_conf(&conf, "pcnode1");
    my_node_conf_t *second = get_my_node_conf(&conf, "pcother1");
    CHECK(first && first->powercap == 250, "node under the first EARGM gets 1000 W / 4 (got %u)",
          first ? first->powercap : 0);
    CHECK(second && second->powercap == 1500, "node under the second EARGM gets 3000 W / 2 (got %u)",
          second ? second->powercap : 0);
}

/* BASELINE_POWERCAP is the tag key; POWERCAP a deprecated alias it wins over */
TEST(it_conf_baseline_powercap_key)
{
    char path[] = "/tmp/pc_it_ear_conf_XXXXXX";
    int fd      = mkstemp(path);
    dprintf(fd, "Tag=newkey default=yes baseline_powercap=700\n"
                "Tag=oldkey powercap=800\n"
                "Tag=both powercap=900 baseline_powercap=600 min_working_power=450\n"
                "Island=0 Nodes=pcn1 tag=newkey\n"
                "Island=0 Nodes=pcn2 tag=oldkey\n"
                "Island=0 Nodes=pcn3 tag=both\n");
    close(fd);
    cluster_conf_t conf;
    memset(&conf, 0, sizeof(conf));
    read_cluster_conf(path, &conf);
    unlink(path);
    my_node_conf_t *n1 = get_my_node_conf(&conf, "pcn1");
    my_node_conf_t *n2 = get_my_node_conf(&conf, "pcn2");
    my_node_conf_t *n3 = get_my_node_conf(&conf, "pcn3");
    CHECK(n1 && n1->powercap == 700, "baseline_powercap=700 (got %u)", n1 ? n1->powercap : 0);
    CHECK(n2 && n2->powercap == 800, "deprecated powercap=800 still accepted (got %u)", n2 ? n2->powercap : 0);
    CHECK(n3 && n3->powercap == 600, "baseline_powercap wins over powercap on the same line (got %u)",
          n3 ? n3->powercap : 0);
    CHECK(n3 && n3->min_working_power == 450, "min_working_power=450 parsed (got %u)",
          n3 ? n3->min_working_power : 0);
}

TEST(it_conf_max_below_default)
{
    /* ear.conf: default powercap 2000 W but max powercap 1500 W */
    fake_pc_max = 1500;
    fake_powermon_reset();
    fake_pc_def = DEF;
    it_setup_noreset(DEF);
    CHECK_EQ(my_pc_opt.current_pc, 1500, "default above max: the node is capped at the max");
    CHECK(hw_total() <= 1500 + gpus * 0, "hardware within 1500 W (%d W)", hw_total());
}

TEST(it_gpu_throttled_at_limit)
{
    if (!gpus) {
        CHECK(1, "no GPUs in this rig");
        return;
    }
    /* gpu.so caps through NVML; the firmware lowers the clock to hold the cap,
     * so gpu.so sees the GPUs under their target and should ask for power */
    it_setup(1200);
    fake_ctx_start(1, 3000000, 1980000, gpus);
    powercap_idle_to_run();
    powercap_new_job();
    wait_gpu_load();
    sleep(2);
    int c0 = hw_gpu_clock(0);
    printf("        GPU0 cap %d W, clock %d MHz (target 1980), power %d W\n", hw_gpu(0), c0, hw_gpu_power(0));
    CHECK(c0 < 1980, "firmware throttles GPU0 to hold its cap (%d MHz)", c0);
    eargm_reply_t r;
    eargm_get_status(&r, 0);
    CHECK(r.pmgt.status == PC_STATUS_GREEDY || r.pmgt.status == PC_STATUS_OK,
          "throttled GPUs do not report RELEASE (status %u)", r.pmgt.status);
    int at_max = 1;
    for (int g = 0; g < gpus; g++)
        at_max &= hw_gpu(g) >= hw_gpu_max(g);
    print_hw("throttled at limit:");
    if (at_max) {
        /* the firmware throttles at the hardware max cap: more watts cannot help */
        CHECK(r.pmgt.status != PC_STATUS_GREEDY && r.pmgt.requested == 0,
              "GPUs throttled at their hardware max do not ask for power (status %u, requested %u)", r.pmgt.status,
              r.pmgt.requested);
    }
    eargm_reply_free(&r);
    powercap_end();

    /* Caps under the hardware max: the GPUs can use more, so they ask for it in watts */
    it_setup_noreset(700);
    eargm_set_powerlimit(700);
    fake_ctx_start(1, 3000000, 1980000, gpus);
    powercap_idle_to_run();
    powercap_new_job();
    sleep(3);
    c0 = hw_gpu_clock(0);
    printf("        GPU0 cap %d W (max %d), clock %d MHz, power %d W\n", hw_gpu(0), hw_gpu_max(0), c0,
           hw_gpu_power(0));
    eargm_get_status(&r, 0);
    CHECK(hw_gpu(0) < hw_gpu_max(0) && c0 < 1980, "GPU0 throttled under its max cap (%d W, %d MHz)", hw_gpu(0), c0);
    CHECK(r.pmgt.status == PC_STATUS_GREEDY && r.pmgt.requested > 0,
          "throttled GPUs make the node GREEDY with a request in watts (status %u, requested %u)", r.pmgt.status,
          r.pmgt.requested);
    eargm_reply_free(&r);
}

static void start_eard_with_busy_gpus(void)
{
    it_setup(DEF);
    settle();
}

TEST(it_eard_start_with_busy_gpus)
{
    if (!busy_at_start()) {
        CHECK(1, "(only in the busy-at-start rig)");
        return;
    }
    /* util detection is subscribed in pmgt_init() before pmgt_enable() enables
     * the plugins; cpu_generic's enable spins ~1 s measuring the base
     * frequency, and a GPU utilization change in that window reaches
     * gpu.so:set_new_utilization() with its arrays still NULL */
    int sig = pc_crashes(start_eard_with_busy_gpus);
    CHECK(sig == 0, "EARD powercap starts on a node whose GPUs are busy (signal %d = %s)", sig,
          sig ? strsignal(sig) : "none");
}

TEST(it_plugin_symbols)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/cpu_generic.so", getenv("IT_PCDIR"));
    void *h = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    CHECK(h != NULL, "cpu_generic.so loads");
    if (h) {
        CHECK(dlsym(h, "get_powercap_strategy") != NULL, "cpu_generic.so exports get_powercap_strategy");
        CHECK(dlsym(h, "plugin_get_settings") != NULL, "cpu_generic.so exports plugin_get_settings");
    }
    snprintf(path, sizeof(path), "%s/gpu.so", getenv("IT_PCDIR"));
    h = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    CHECK(h != NULL && dlsym(h, "get_powercap_status") != NULL, "gpu.so loads and exports get_powercap_status");
}

int main(void)
{
    const char *only_rig = getenv("IT_ONLY");
    (void) only_rig;
    gpus = getenv("IT_GPUS") ? atoi(getenv("IT_GPUS")) : 0;
    const char *u = getenv("IT_GPU_UTIL");
    for (int g = 0; g < gpus && u; g++) {
        gpu_util[g] = (uint) atoi(u);
        u           = strchr(u, ',');
        if (u)
            u++;
    }
    /* intel63 programs each socket through the first CPU EAR selects for it */
    topology_t tp, tps;
    topology_init(&tp);
    topology_select(&tp, &tps, TPSelect.socket, TPGroup.merge, 0);
    for (int s = 0; s < SOCKETS && s < tps.cpu_count; s++)
        socket_cpu[s] = tps.cpus[s].id;
    printf("rig: %d sockets (PKG %d W, DRAM %d W), %d GPUs util [%s]\n", SOCKETS, PKG_TDP, DRAM_TDP, gpus,
           getenv("IT_GPU_UTIL") ? getenv("IT_GPU_UTIL") : "");
    ips[0] = 0x0a000001;
    if (getenv("IT_VERB"))
        VERB_SET_LV(atoi(getenv("IT_VERB")));

    if (busy_at_start()) {
        RUN_ISOLATED(it_eard_start_with_busy_gpus);
        return pc_summary("test_integration");
    }
    if (limits_rig()) {
        RUN_ISOLATED(it_topology_seen_by_ear);
        RUN_ISOLATED(it_cap_below_hardware_minimum);
        RUN_ISOLATED(it_cap_above_hardware_maximum);
        RUN_ISOLATED(it_device_caps_outside_hardware_range);
        RUN_ISOLATED(it_conf_auto_powercap_first_eargm);
        RUN_ISOLATED(it_conf_baseline_powercap_key);
        RUN_ISOLATED(it_conf_max_below_default);
        RUN_ISOLATED(it_gpu_throttled_at_limit);
        return pc_summary("test_integration");
    }
    if (enforce_rig()) {
        RUN_ISOLATED(it_topology_seen_by_ear);
        RUN_ISOLATED(it_generic_throttled_node_asks_for_power);
        RUN_ISOLATED(it_generic_release_matches_headroom);
        RUN_ISOLATED(it_node_stays_under_cap_through_eargm);
        return pc_summary("test_integration");
    }
    if (dvfs_rig()) {
        RUN_ISOLATED(it_topology_seen_by_ear);
        RUN_ISOLATED(it_dvfs_holds_cpu_limit);
        RUN_ISOLATED(it_gpu_dvfs_holds_gpu_budget);
        return pc_summary("test_integration");
    }
    RUN_ISOLATED(it_topology_seen_by_ear);
    RUN_ISOLATED(it_initial_caps);
    RUN_ISOLATED(it_verify_after_init);
    RUN_ISOLATED(it_gpu_split_follows_load);
    RUN_ISOLATED(it_soft_powercap_sequence);
    RUN_ISOLATED(it_limit_clamped_to_max);
    RUN_ISOLATED(it_tiny_limit);
    RUN_ISOLATED(it_manual_per_device);
    RUN_ISOLATED(it_eargm_cycles_keep_node_under_cap);
    RUN_ISOLATED(it_plugin_symbols);
    return pc_summary("test_integration");
}
