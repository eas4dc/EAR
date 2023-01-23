CC_FLAGS += $(CONSTANTS)
CFLAGS = $(CC_FLAGS) -fPIC -shared -Wextra 

######## INSTALLATION

stat_BINS = libmetrics.a
dyna_BINS = libmetrics.so

######## FILES

mets_OBJS = \
    $(SRCDIR)/metrics/common/apis.o \
    $(SRCDIR)/metrics/common/cupti.o \
    $(SRCDIR)/metrics/common/hsmp.o \
    $(SRCDIR)/metrics/common/hwmon.o \
    $(SRCDIR)/metrics/common/isst.o \
    $(SRCDIR)/metrics/common/likwid.o \
    $(SRCDIR)/metrics/common/nvml.o \
    $(SRCDIR)/metrics/common/pci.o \
    $(SRCDIR)/metrics/common/pstate.o \
    $(SRCDIR)/metrics/common/msr.o \
    $(SRCDIR)/metrics/common/omsr.o \
    $(SRCDIR)/metrics/common/oneapi.o \
    $(SRCDIR)/metrics/common/perf.o \
    $(SRCDIR)/metrics/common/redfish.o \
    $(SRCDIR)/metrics/accumulators/power_metrics.o \
    $(SRCDIR)/metrics/bandwidth/bandwidth.o \
    $(SRCDIR)/metrics/bandwidth/archs/amd17.o \
    $(SRCDIR)/metrics/bandwidth/archs/bypass.o \
    $(SRCDIR)/metrics/bandwidth/archs/dummy.o \
    $(SRCDIR)/metrics/bandwidth/archs/eard.o \
    $(SRCDIR)/metrics/bandwidth/archs/intel63.o \
    $(SRCDIR)/metrics/bandwidth/archs/intel106.o \
    $(SRCDIR)/metrics/bandwidth/archs/likwid.o \
    $(SRCDIR)/metrics/cache/cache.o \
    $(SRCDIR)/metrics/cache/archs/dummy.o \
    $(SRCDIR)/metrics/cache/archs/perf.o \
    $(SRCDIR)/metrics/cpi/cpi.o \
    $(SRCDIR)/metrics/cpi/archs/dummy.o \
    $(SRCDIR)/metrics/cpi/archs/perf.o \
    $(SRCDIR)/metrics/energy/cpu.o \
    $(SRCDIR)/metrics/energy/energy_node.o \
    $(SRCDIR)/metrics/energy/cpu/intel63.o \
    $(SRCDIR)/metrics/flops/flops.o \
    $(SRCDIR)/metrics/flops/archs/perf.o \
    $(SRCDIR)/metrics/flops/archs/dummy.o \
    $(SRCDIR)/metrics/cpufreq/cpufreq.o \
    $(SRCDIR)/metrics/cpufreq/cpufreq_base.o \
    $(SRCDIR)/metrics/cpufreq/archs/eard.o \
    $(SRCDIR)/metrics/cpufreq/archs/dummy.o \
    $(SRCDIR)/metrics/cpufreq/archs/intel63.o \
    $(SRCDIR)/metrics/imcfreq/imcfreq.o \
    $(SRCDIR)/metrics/imcfreq/archs/amd17.o \
    $(SRCDIR)/metrics/imcfreq/archs/eard.o \
    $(SRCDIR)/metrics/imcfreq/archs/dummy.o \
    $(SRCDIR)/metrics/imcfreq/archs/intel63.o \
	$(SRCDIR)/metrics/proc/stat.o \
    $(SRCDIR)/metrics/gpu/gpu.o \
    $(SRCDIR)/metrics/gpu/gpuproc.o \
    $(SRCDIR)/metrics/gpu/archs/cupti.o \
    $(SRCDIR)/metrics/gpu/archs/dummy.o \
    $(SRCDIR)/metrics/gpu/archs/eard.o \
    $(SRCDIR)/metrics/gpu/archs/nvml.o \
    $(SRCDIR)/metrics/gpu/archs/oneapi.o \
    $(SRCDIR)/metrics/temperature/temperature.o \
    $(SRCDIR)/metrics/temperature/archs/amd17.o \
    $(SRCDIR)/metrics/temperature/archs/intel63.o \
    $(SRCDIR)/metrics/temperature/archs/dummy.o \
	$(SRCDIR)/metrics/energy_cpu/energy_cpu.o \
	$(SRCDIR)/metrics/energy_cpu/archs/msr.o \
	$(SRCDIR)/metrics/energy_cpu/archs/eard.o \
	$(SRCDIR)/metrics/energy_cpu/archs/dummy.o \
	$(SRCDIR)/metrics/energy_cpu/archs/cpu_util.o \
    $(SRCDIR)/metrics/io/io.o

######## RULES

all: all.components $(stat_BINS) $(dyna_BINS)

post: test
	@ $(MAKE) -C 'gpu' post || exit

libmetrics.a: $(mets_OBJS)
	$(AR) rvs $@ $(mets_OBJS)

libmetrics.so: $(mets_OBJS)
	$(CC) $(CFLAGS) -o $@ $(mets_OBJS)

test: test.c
	$(CC) -I $(SRCDIR) -DTEST=1 -fPIC -o $@ $^ libmetrics.a ../daemon/local_api/libeard.a ../common/libcommon.a -lm -lpthread -ldl

######## OPTIONS

install: install.components;

devel.install: devel.install.components;

%.install: %.install.components;

clean: clean.components rclean
	rm -f test

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'accumulators' $* || exit
	@ $(MAKE) -C 'bandwidth'    $* || exit
	@ $(MAKE) -C 'cache'        $* || exit
	@ $(MAKE) -C 'common'       $* || exit
	@ $(MAKE) -C 'gpu'          $* || exit
	@ $(MAKE) -C 'cpi'          $* || exit
	@ $(MAKE) -C 'cpufreq'      $* || exit
	@ $(MAKE) -C 'energy'       $* || exit
	@ $(MAKE) -C 'flops'        $* || exit
	@ $(MAKE) -C 'imcfreq'      $* || exit
	@ $(MAKE) -C 'io'           $* || exit
	@ $(MAKE) -C 'rapl'         $* || exit
	@ $(MAKE) -C 'proc'         $* || exit
	@ $(MAKE) -C 'temperature'  $* || exit
	@ $(MAKE) -C 'energy_cpu'   $* || exit

######## DEPENDENCIES

depend: depend.components

depend-clean: depend-clean.components

include $(SRCDIR)/Makefile.extra
