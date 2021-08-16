CFLAGS = $(CC_FLAGS) -fPIC -shared -O2 -Wextra

######## INSTALLATION

stat_BINS = libmetrics.a
dyna_BINS = libmetrics.so

######## FILES

mets_OBJS = \
    $(SRCDIR)/metrics/common/apis.o \
    $(SRCDIR)/metrics/common/hsmp.o \
    $(SRCDIR)/metrics/common/hwmon.o \
    $(SRCDIR)/metrics/common/pci.o \
    $(SRCDIR)/metrics/common/pstate.o \
    $(SRCDIR)/metrics/common/msr.o \
    $(SRCDIR)/metrics/common/omsr.o \
    $(SRCDIR)/metrics/common/perf.o \
    $(SRCDIR)/metrics/accumulators/power_metrics.o \
    $(SRCDIR)/metrics/bandwidth/bandwidth.o \
    $(SRCDIR)/metrics/bandwidth/cpu/amd49.o \
    $(SRCDIR)/metrics/bandwidth/cpu/intel63.o \
    $(SRCDIR)/metrics/bandwidth/cpu/dummy.o \
    $(SRCDIR)/metrics/cache/cache.o \
    $(SRCDIR)/metrics/cache/cpu/perf.o \
    $(SRCDIR)/metrics/cpi/cpi.o \
    $(SRCDIR)/metrics/cpi/cpu/intel63.o \
    $(SRCDIR)/metrics/cpi/cpu/dummy.o \
    $(SRCDIR)/metrics/energy/cpu.o \
    $(SRCDIR)/metrics/energy/energy_node.o \
    $(SRCDIR)/metrics/energy/energy_node_lib.o \
    $(SRCDIR)/metrics/energy/cpu/intel63.o \
    $(SRCDIR)/metrics/flops/flops.o \
    $(SRCDIR)/metrics/flops/cpu/amd49.o \
    $(SRCDIR)/metrics/flops/cpu/intel63.o \
    $(SRCDIR)/metrics/flops/cpu/dummy.o \
    $(SRCDIR)/metrics/frequency/cpu.o \
    $(SRCDIR)/metrics/frequency/cpufreq_base.o \
    $(SRCDIR)/metrics/frequency/cpu/intel63.o \
    $(SRCDIR)/metrics/frequency/cpu/intel_dummy.o \
    $(SRCDIR)/metrics/imcfreq/imcfreq.o \
    $(SRCDIR)/metrics/imcfreq/archs/amd17.o \
    $(SRCDIR)/metrics/imcfreq/archs/eard.o \
    $(SRCDIR)/metrics/imcfreq/archs/dummy.o \
    $(SRCDIR)/metrics/imcfreq/archs/intel63.o \
	$(SRCDIR)/metrics/proc/stat.o \
    $(SRCDIR)/metrics/gpu/gpu.o \
    $(SRCDIR)/metrics/gpu/archs/nvml.o \
    $(SRCDIR)/metrics/gpu/archs/dummy.o \
    $(SRCDIR)/metrics/temperature/temperature.o \
    $(SRCDIR)/metrics/temperature/archs/amd17.o \
    $(SRCDIR)/metrics/temperature/archs/intel63.o \
    $(SRCDIR)/metrics/temperature/archs/dummy.o \
    $(SRCDIR)/metrics/io/io.o

######## RULES

all: all.components $(stat_BINS) $(dyna_BINS)

libmetrics.a: $(mets_OBJS)
	$(AR) rvs $@ $(mets_OBJS)

libmetrics.so: $(mets_OBJS)
	$(CC) $(CFLAGS) -o $@ $(mets_OBJS)

######## OPTIONS

install: install.components;

devel.install: devel.install.components;

%.install: %.install.components;

clean: clean.components rclean;

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'accumulators' $* || exit
	@ $(MAKE) -C 'bandwidth'    $* || exit
	@ $(MAKE) -C 'cache'        $* || exit
	@ $(MAKE) -C 'common'       $* || exit
	@ $(MAKE) -C 'cpi'          $* || exit
	@ $(MAKE) -C 'energy'       $* || exit
	@ $(MAKE) -C 'flops'        $* || exit
	@ $(MAKE) -C 'frequency'    $* || exit
	@ $(MAKE) -C 'gpu'          $* || exit
	@ $(MAKE) -C 'imcfreq'      $* || exit
	@ $(MAKE) -C 'io'           $* || exit
	@ $(MAKE) -C 'proc'         $* || exit
	@ $(MAKE) -C 'temperature'  $* || exit

######## DEPENDENCIES

depend: depend.components

depend-clean: depend-clean.components

include $(SRCDIR)/Makefile.extra
