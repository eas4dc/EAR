######## INSTALLATION

######## FILES

mets_OBJS = \
    $(SRCDIR)/metrics/common/apis.o \
    $(SRCDIR)/metrics/common/cupti.o \
    $(SRCDIR)/metrics/common/file.o \
    $(SRCDIR)/metrics/common/hsmp.o \
    $(SRCDIR)/metrics/common/hwmon.o \
    $(SRCDIR)/metrics/common/isst.o \
    $(SRCDIR)/metrics/common/likwid.o \
    $(SRCDIR)/metrics/common/nvml.o \
    $(SRCDIR)/metrics/common/pci.o \
    $(SRCDIR)/metrics/common/msr.o \
    $(SRCDIR)/metrics/common/omsr.o \
    $(SRCDIR)/metrics/common/oneapi.o \
    $(SRCDIR)/metrics/common/perf.o \
    $(SRCDIR)/metrics/common/pstate.o \
    $(SRCDIR)/metrics/common/rsmi.o \
    $(SRCDIR)/metrics/common/redfish.o \
    $(SRCDIR)/metrics/common/cray_pm_counters.o \
    $(SRCDIR)/metrics/accumulators/power_metrics.o \
    $(SRCDIR)/metrics/bandwidth/bandwidth.o \
    $(SRCDIR)/metrics/bandwidth/archs/amd17.o \
    $(SRCDIR)/metrics/bandwidth/archs/amd17df.o \
    $(SRCDIR)/metrics/bandwidth/archs/amd19.o \
    $(SRCDIR)/metrics/bandwidth/archs/dummy.o \
    $(SRCDIR)/metrics/bandwidth/archs/eard.o \
    $(SRCDIR)/metrics/bandwidth/archs/intel63.o \
    $(SRCDIR)/metrics/bandwidth/archs/intel106.o \
    $(SRCDIR)/metrics/bandwidth/archs/intel143.o \
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
    $(SRCDIR)/metrics/energy_cpu/energy_cpu.o \
    $(SRCDIR)/metrics/energy_cpu/archs/cpu_util.o \
    $(SRCDIR)/metrics/energy_cpu/archs/linux_powercap.o \
    $(SRCDIR)/metrics/energy_cpu/archs/dummy.o \
    $(SRCDIR)/metrics/energy_cpu/archs/linux_powercap.o \
    $(SRCDIR)/metrics/energy_cpu/archs/eard.o \
    $(SRCDIR)/metrics/energy_cpu/archs/msr.o \
    $(SRCDIR)/metrics/energy_cpu/archs/perf.o \
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
    $(SRCDIR)/metrics/gpu/gpuprof.o \
    $(SRCDIR)/metrics/gpu/archs/cupti.o \
    $(SRCDIR)/metrics/gpu/archs/dcgmi.o \
    $(SRCDIR)/metrics/gpu/archs/dummy.o \
    $(SRCDIR)/metrics/gpu/archs/eard.o \
    $(SRCDIR)/metrics/gpu/archs/nvml.o \
    $(SRCDIR)/metrics/gpu/archs/oneapi.o \
    $(SRCDIR)/metrics/gpu/archs/pvc_power_hwmon.o \
    $(SRCDIR)/metrics/gpu/archs/rsmi.o \
    $(SRCDIR)/metrics/gpu/archs/gpuprof_nvml.o \
    $(SRCDIR)/metrics/gpu/archs/gpuprof_dcgmi.o \
    $(SRCDIR)/metrics/gpu/archs/gpuprof_dummy.o \
    $(SRCDIR)/metrics/temperature/temperature.o \
    $(SRCDIR)/metrics/temperature/archs/amd17.o \
    $(SRCDIR)/metrics/temperature/archs/eard.o \
    $(SRCDIR)/metrics/temperature/archs/intel63.o \
    $(SRCDIR)/metrics/temperature/archs/dummy.o \
    $(SRCDIR)/metrics/io/io.o

######## RULES

all: all.components metrics.o libmetrics.a 

post: test
	@ $(MAKE) -C 'gpu' post || exit
	@ $(MAKE) -C 'common' post || exit

metrics.o: metrics.c
	$(CC) $(CC_FLAGS) -I$(SRCDIR) -fPIC -c $<

libmetrics.a: metrics.o $(mets_OBJS)
	$(AR) rvs $@ $^

test: metrics.c
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
	@ $(MAKE) -C 'flops'        $* || exit
	@ $(MAKE) -C 'imcfreq'      $* || exit
	@ $(MAKE) -C 'io'           $* || exit
	@ $(MAKE) -C 'proc'         $* || exit
	@ $(MAKE) -C 'temperature'  $* || exit
	@ $(MAKE) -C 'energy_cpu'   $* || exit
	@ $(MAKE) -C 'energy'       $* || exit


######## DEPENDENCIES

depend: depend.components

depend-clean: depend-clean.components

include $(SRCDIR)/Makefile.extra
