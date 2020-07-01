CFLAGS = $(CC_FLAGS) -fPIC -shared -O2 -Wextra

######## INSTALLATION

stat_BINS = libmetrics.a
dyna_BINS = libmetrics.so

######## FILES

mets_OBJS = \
    $(SRCDIR)/metrics/common/pci.o \
    $(SRCDIR)/metrics/common/msr.o \
    $(SRCDIR)/metrics/common/papi.o \
    $(SRCDIR)/metrics/accumulators/power_metrics.o \
    $(SRCDIR)/metrics/bandwidth/bandwidth.o \
    $(SRCDIR)/metrics/bandwidth/cpu/intel_haswell.o \
    $(SRCDIR)/metrics/bandwidth/cpu/utils.o \
    $(SRCDIR)/metrics/cache/cache.o \
    $(SRCDIR)/metrics/cache/cpu/papi.o \
    $(SRCDIR)/metrics/cpi/cpi.o \
    $(SRCDIR)/metrics/cpi/cpu/papi.o \
    $(SRCDIR)/metrics/energy/energy_cpu.o \
    $(SRCDIR)/metrics/energy/energy_gpu.o \
    $(SRCDIR)/metrics/energy/energy_node.o \
    $(SRCDIR)/metrics/energy/energy_node_lib.o \
    $(SRCDIR)/metrics/energy/cpu/intel_haswell.o \
    $(SRCDIR)/metrics/energy/gpu/nvsmi.o \
    $(SRCDIR)/metrics/flops/flops.o \
    $(SRCDIR)/metrics/flops/cpu/papi.o \
    $(SRCDIR)/metrics/frequency/frequency_imc.o \
    $(SRCDIR)/metrics/frequency/frequency_cpu.o \
    $(SRCDIR)/metrics/frequency/cpu/intel_haswell.o \
    $(SRCDIR)/metrics/frequency/imc/intel_haswell.o \
    $(SRCDIR)/metrics/stalls/stalls.o \
    $(SRCDIR)/metrics/stalls/cpu/papi.o \
    $(SRCDIR)/metrics/temperature/temperature.o

# $(SRCDIR)/metrics/energy/cpu/intel_haswell.o \

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
	@ $(MAKE) -C 'common'       $* || exit
	@ $(MAKE) -C 'bandwidth'    $* || exit
	@ $(MAKE) -C 'cache'        $* || exit
	@ $(MAKE) -C 'cpi'          $* || exit
	@ $(MAKE) -C 'energy'       $* || exit
	@ $(MAKE) -C 'flops'        $* || exit
	@ $(MAKE) -C 'frequency'    $* || exit
	@ $(MAKE) -C 'stalls'       $* || exit
	@ $(MAKE) -C 'accumulators' $* || exit
	@ $(MAKE) -C 'temperature' $* || exit

######## DEPENDENCIES

depend: depend.components

depend-clean: depend-clean.components

include $(SRCDIR)/Makefile.extra
