#!/usr/bin/env bash
#
# Runs test_integration once per rig, each on its own fake hardware.
#
#   FAKEDEV  ear-fake-devices checkout (built: make)
#   RIGS     optional space-separated list of rigs to run (default: all)
#   PCDIR    src/daemon/powercap of a built EAR tree (cpu_generic.so, gpu.so)
#
# Rigs: 2 sockets (PKG 200 W, DRAM 40 W) with
#   gpu4-mixed          4 GPUs at 100/50/25/0 % utilization
#   gpu4-equal          4 GPUs at 80 % each
#   gpu2                2 GPUs at 90/30 %
#   cpu-only            no GPUs
#   gpu4-busy-at-start  4 GPUs already at 80 % when EARD starts
#   dvfs-gpu4           the DEFAULT plugins, dvfs.so + gpu_dvfs.so, 4 GPUs at 80 %
#   dvfs-cpu-only       dvfs.so alone
#   enforce-cpu-only    cpu_generic.so with RAPL enforcing its caps (closed loop)
#   enforce-gpu4        cpu_generic.so + gpu.so, RAPL and GPU limits enforced
#   limits-cpu-only     enforce + RAPL min/max power (PKG 60..250 W, DRAM 10..60 W)
#   limits-gpu4         same with 4 GPUs (NVML 100..400 W)
set -u
cd "$(dirname "$0")"

FAKEDEV=${FAKEDEV:-$HOME/ear-fake-devices}
PCDIR=${PCDIR:-$(cd ../../../src/daemon/powercap && pwd)}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

for lib in libfakesys.so libfakenvml.so; do
    if [ ! -f "$FAKEDEV/$lib" ]; then
        echo "missing $FAKEDEV/$lib (run make in $FAKEDEV)"; exit 2
    fi
done
for plug in cpu_generic.so gpu.so; do
    if [ ! -f "$PCDIR/$plug" ]; then
        echo "missing $PCDIR/$plug (build EAR first)"; exit 2
    fi
done

cat > "$TMP/sys.conf" <<'EOF'
rapl            = 1
rapl_packages   = 2
topo_sockets    = 2
rapl_power_exp  = 3
rapl_tdp        = 200
rapl_dram_tdp   = 40
rapl_pkg_wave   = const
rapl_pkg_power  = 150
rapl_dram_power = 25
cray  = 0
hwmon = 0
hsmp  = 0
isst  = 0
fake_uid = 0
EOF

# The default plugins (dvfs.so, gpu_dvfs.so) cap by frequency, so their rigs
# fake the cpufreq sysfs and make power follow the clocks EAR sets.
{
    cat "$TMP/sys.conf"
    echo "cpufreq_sysfs      = 1"
    echo "cpufreq_max_khz    = 3000000"
    echo "cpufreq_min_khz    = 1000000"
    echo "cpufreq_step_khz   = 100000"
    echo "rapl_pkg_w_per_ghz = 50"
} > "$TMP/sys-dvfs.conf"
# cpu_generic.so + gpu.so closed loop: RAPL and the GPU firmware enforce the
# caps EAR programs, and power follows frequency/clock.
{
    cat "$TMP/sys-dvfs.conf"
    echo "rapl_enforce       = 1"
} > "$TMP/sys-enforce.conf"
# Same, with the RAPL power range reported and held by the firmware.
{
    cat "$TMP/sys-enforce.conf"
    echo "rapl_pkg_min       = 60"
    echo "rapl_pkg_max       = 250"
    echo "rapl_dram_min      = 10"
    echo "rapl_dram_max      = 60"
} > "$TMP/sys-limits.conf"

# GPUs start idle and reach their utilization after IDLE_S seconds, like a job
# starting on a node where EARD is already running. busy-at-start rigs start
# loaded instead (EARD restarted under a running job).
IDLE_S=6

rig() { # name gpus util-list [busy-at-start] [cpu-plugin gpu-plugin] [enforce] [limits]
    local name=$1 n=$2 util=$3 busy=${4:-0} cplug=${5:-cpu_generic} gplug=${6:-gpu} enforce=${7:-0} limits=${8:-0}
    if [ -n "${RIGS:-}" ] && ! [[ " $RIGS " == *" $name "* ]]; then return 0; fi
    local sysconf="$TMP/sys.conf"
    [ "$cplug" = dvfs ] && sysconf="$TMP/sys-dvfs.conf"
    [ "$enforce" = 1 ] && sysconf="$TMP/sys-enforce.conf"
    [ "$limits" = 1 ] && sysconf="$TMP/sys-limits.conf"
    {
        echo "gpus          = $n"
        echo "power_min     = 100"
        echo "power_max     = 400"
        echo "power_default = 400"
        echo "power_limit   = 400"
        echo "gpu_wave      = const"
        echo "gpu_power     = 250"
        if [ "$gplug" = gpu_dvfs ] || [ "$enforce" = 1 ]; then
            echo "power_model   = clock"
            echo "idle_power    = 50"
            echo "w_per_mhz     = 0.2"
            if [ "$limits" = 1 ]; then echo "sm_clocks     = 1980:210:15"   # lowest clock fits the 100 W minimum
            else echo "sm_clocks     = 1980:780:15"; fi
            echo "sm_mhz        = 1980"
        fi
        [ "$enforce" = 1 ] && echo "enforce_limit = 1"
        local i=0
        IFS=, read -ra u <<< "$util"
        for v in "${u[@]}"; do
            if [ "$busy" = 1 ]; then echo "gpu${i}_util = $v"
            else echo "gpu${i}_util_steps = $IDLE_S:0, 100000:$v"; fi
            i=$((i + 1))
        done
    } > "$TMP/nvml-$name.conf"

    echo
    echo "################################################## rig $name"
    env LD_PRELOAD="$FAKEDEV/libfakesys.so" FAKESYS_CONF="$sysconf" FAKESYS_ROOT="$TMP/root-$name" \
        HACK_NVML_FILE="$FAKEDEV/libfakenvml.so" FAKENVML_CONF="$TMP/nvml-$name.conf" \
        IT_GPUS="$n" IT_GPU_UTIL="$util" IT_PCDIR="$PCDIR" IT_IDLE_S="$IDLE_S" IT_BUSY_AT_START="$busy" \
        IT_CPU_PLUGIN="$cplug" IT_GPU_PLUGIN="$gplug" IT_ENFORCE="$enforce" IT_LIMITS="$limits" \
        IT_GPU_CLK_MIN="$([ "$limits" = 1 ] && echo 210 || echo 780)" \
        ./test_integration 2>"${IT_LOGDIR:-$TMP}/stderr-$name.log"
}

rc=0
rig gpu4-mixed 4 100,50,25,0 || rc=1
rig gpu4-equal 4 80,80,80,80 || rc=1
rig gpu2       2 90,30       || rc=1
rig cpu-only   0 ""          || rc=1
rig gpu4-busy-at-start 4 80,80,80,80 1 || rc=1
rig dvfs-gpu4 4 80,80,80,80 0 dvfs gpu_dvfs || rc=1
rig dvfs-cpu-only 0 "" 0 dvfs gpu_dvfs || rc=1
rig enforce-cpu-only 0 "" 0 cpu_generic gpu 1 || rc=1
rig enforce-gpu4 4 80,80,80,80 0 cpu_generic gpu 1 || rc=1
rig limits-cpu-only 0 "" 0 cpu_generic gpu 1 1 || rc=1
rig limits-gpu4 4 80,80,80,80 0 cpu_generic gpu 1 1 || rc=1
exit $rc
