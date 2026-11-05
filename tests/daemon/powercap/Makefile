# Tests for the EARD powercap (src/daemon/powercap) in isolation from EARGM.
#
#   make unit          unit tier: needs only a configured tree (config_install.h)
#   make integration   integration tier: needs a built tree (libcommon/libmetrics/
#                      libmgt and the powercap plugins) and the fake devices repo
#   make check         both
#
# FAKEDEV points to the ear-fake-devices checkout.

TOPDIR  = $(shell cd ../../.. && pwd)
SRCDIR  = $(TOPDIR)/src
PCDIR   = $(SRCDIR)/daemon/powercap
FAKEDEV ?= $(HOME)/ear-fake-devices

CC      ?= gcc
CFLAGS  = -g -O0 -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable \
          -I$(SRCDIR) -I. -DUSE_GPUS=1
LDLIBS  = -lpthread -lm -ldl

UNIT_TESTS = test_status test_node test_monitor test_mgt test_gpu_plugin test_gpu_dvfs_plugin \
             test_gpu_dvfs_plugin_asan test_cpu_plugin test_cpu_plugin_asan test_dvfs_plugin \
             test_inm_plugin test_grace_hopper_plugin test_gpu_dummy_plugin test_pvc_hwmon
INTEG_TESTS = test_integration

HDRS = $(wildcard *.h)

all: $(UNIT_TESTS)

$(UNIT_TESTS) $(INTEG_TESTS): $(HDRS)

# --------------------------------------------------------------- unit tier

test_status: test_status.c $(PCDIR)/powercap_status.c $(SRCDIR)/common/types/powercap.c
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) $(LDLIBS)

# powercap.c (node state machine + EARGM entry points) against a recording
# fake of the pmgt layer and a fake power monitor.
test_node: test_node.c fake_pmgt.c fake_powermon.c common_stubs.c eargm_sim.c \
           $(PCDIR)/powercap.c $(PCDIR)/powercap_status.c $(SRCDIR)/common/system/lock.c
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) $(LDLIBS)

# The same powercap.c built with the optional device-verification monitor.
test_monitor: test_monitor.c fake_pmgt.c fake_powermon.c common_stubs.c \
              $(PCDIR)/powercap.c $(PCDIR)/powercap_status.c $(SRCDIR)/common/system/lock.c
	$(CC) $(CFLAGS) -DPOWERCAP_MON=1 -o $@ $(filter %.c,$^) $(LDLIBS)

# powercap_mgt.c (distribution between domains) against in-process fake
# domain plugins and stubbed hardware discovery.
test_mgt: test_mgt.c fake_hw_mgt.c fake_powermon.c common_stubs.c \
          $(PCDIR)/powercap_mgt.c $(PCDIR)/powercap_status.c
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) $(LDLIBS)

# The GPU plugin (gpu.c) against stubbed management/metrics GPU APIs.
test_gpu_plugin: test_gpu_plugin.c fake_dev_api.c common_stubs.c \
                 $(PCDIR)/gpu.c $(PCDIR)/powercap_status.c
	$(CC) $(CFLAGS) -DFAKE_GPU_SIDE -o $@ $(filter %.c,$^) $(LDLIBS)

# The CPU plugin (cpu_generic.c) against stubbed cpupow/cpufreq/energy APIs.
# -rdynamic so the test can dlsym() the plugin's exported symbol names;
# ASan to catch reads past the P-state list.
test_cpu_plugin: test_cpu_plugin.c fake_dev_api.c common_stubs.c \
                 $(PCDIR)/cpu_generic.c $(PCDIR)/powercap_status.c
	$(CC) $(CFLAGS) -DFAKE_CPU_SIDE -rdynamic -o $@ $(filter %.c,$^) $(LDLIBS)

test_cpu_plugin_asan: test_cpu_plugin.c fake_dev_api.c common_stubs.c \
                 $(PCDIR)/cpu_generic.c $(PCDIR)/powercap_status.c
	$(CC) $(CFLAGS) -DFAKE_CPU_SIDE -rdynamic -fsanitize=address -fno-omit-frame-pointer -o $@ $(filter %.c,$^) $(LDLIBS)

# The default plugins: dvfs.so (CPU) and gpu_dvfs.so (GPU), closed-loop.
test_dvfs_plugin: test_dvfs_plugin.c fake_dev_api.c common_stubs.c \
                  $(PCDIR)/dvfs.c $(PCDIR)/powercap_status.c
	$(CC) $(CFLAGS) -DFAKE_CPU_SIDE -o $@ $(filter %.c,$^) $(LDLIBS)

test_gpu_dvfs_plugin: test_gpu_dvfs_plugin.c fake_dev_api.c common_stubs.c \
                      $(PCDIR)/gpu_dvfs.c $(PCDIR)/powercap_status.c
	$(CC) $(CFLAGS) -DFAKE_GPU_SIDE -o $@ $(filter %.c,$^) $(LDLIBS)

test_gpu_dvfs_plugin_asan: test_gpu_dvfs_plugin.c fake_dev_api.c common_stubs.c \
                           $(PCDIR)/gpu_dvfs.c $(PCDIR)/powercap_status.c
	$(CC) $(CFLAGS) -DFAKE_GPU_SIDE -fsanitize=address -fno-omit-frame-pointer -o $@ $(filter %.c,$^) $(LDLIBS)

# Command-driven plugins, each called through the manager's function types.
test_inm_plugin: test_cmd_plugins.c common_stubs.c $(PCDIR)/inm.c
	$(CC) $(CFLAGS) -DPLUG_INM -o $@ $(filter %.c,$^) $(LDLIBS)
test_grace_hopper_plugin: test_cmd_plugins.c common_stubs.c $(PCDIR)/grace_hopper.c
	$(CC) $(CFLAGS) -DPLUG_GH -o $@ $(filter %.c,$^) $(LDLIBS)
test_gpu_dummy_plugin: test_cmd_plugins.c common_stubs.c $(PCDIR)/gpu_dummy.c
	$(CC) $(CFLAGS) -DPLUG_GPU_DUMMY -o $@ $(filter %.c,$^) $(LDLIBS)

# The PVC (i915 hwmon) GPU management backend against a fake hwmon tree, under ASan:
# its power-cap calls must write one uint32_t per GPU.
test_pvc_hwmon: test_pvc_hwmon.c common_stubs.c $(SRCDIR)/management/gpu/archs/mgt_pvc_power_hwmon.c
	$(CC) $(CFLAGS) -DPVC_HWMON_ROOT=pvc_test_root -include fake_pvc_root.h -fsanitize=address \
	    -fno-omit-frame-pointer -o $@ $(filter %.c,$^) $(LDLIBS)

unit: $(UNIT_TESTS)
	@rc=0; for t in $(UNIT_TESTS); do echo "--- $$t"; ASAN_OPTIONS=abort_on_error=1 ./$$t 2>/dev/null | grep -v '^  PASS' || true; ASAN_OPTIONS=abort_on_error=1 ./$$t >/dev/null 2>&1 || rc=1; done; exit $$rc

# -------------------------------------------------------- integration tier

EAR_LIBS = $(SRCDIR)/daemon/local_api/libeard.a $(SRCDIR)/management/libmgt.a $(SRCDIR)/metrics/libmetrics.a $(SRCDIR)/common/libcommon.a

test_integration: test_integration.c fake_powermon.c eargm_sim.c \
                  $(PCDIR)/powercap.c $(PCDIR)/powercap_mgt.c $(PCDIR)/powercap_status.c $(EAR_LIBS)
	$(CC) $(CFLAGS) -rdynamic -o $@ $(filter %.c,$^) $(EAR_LIBS) $(EAR_LIBS) $(LDLIBS)

integration: test_integration
	FAKEDEV=$(FAKEDEV) PCDIR=$(PCDIR) ./run_integration.sh

check: unit integration

clean:
	rm -f $(UNIT_TESTS) $(INTEG_TESTS) *.o

.PHONY: all unit integration check clean
