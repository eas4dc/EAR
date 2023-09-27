CFLAGS    = $(CC_FLAGS) -I$(SRCDIR) -fPIC -Wall
CC_FLAGS += $(CONSTANTS)

######## FILES

root_BINS = libmgt.a

root_OBJS = \
    $(SRCDIR)/management/cpufreq/cpufreq.o \
    $(SRCDIR)/management/cpufreq/governor.o \
    $(SRCDIR)/management/cpufreq/frequency.o \
    $(SRCDIR)/management/cpufreq/priority.o \
    $(SRCDIR)/management/cpufreq/archs/amd17.o \
    $(SRCDIR)/management/cpufreq/archs/dummy.o \
    $(SRCDIR)/management/cpufreq/archs/default.o \
    $(SRCDIR)/management/cpufreq/archs/eard.o \
    $(SRCDIR)/management/cpufreq/archs/prio_dummy.o \
    $(SRCDIR)/management/cpufreq/archs/prio_eard.o \
    $(SRCDIR)/management/cpufreq/archs/prio_isst.o \
    $(SRCDIR)/management/cpufreq/drivers/acpi_cpufreq.o \
    $(SRCDIR)/management/cpufreq/drivers/intel_pstate.o \
    $(SRCDIR)/management/cpupow/cpupow.o \
    $(SRCDIR)/management/cpupow/archs/amd17.o \
    $(SRCDIR)/management/cpupow/archs/dummy.o \
    $(SRCDIR)/management/cpupow/archs/intel63.o \
    $(SRCDIR)/management/imcfreq/imcfreq.o \
    $(SRCDIR)/management/imcfreq/archs/amd17.o \
    $(SRCDIR)/management/imcfreq/archs/dummy.o \
    $(SRCDIR)/management/imcfreq/archs/eard.o \
    $(SRCDIR)/management/imcfreq/archs/intel63.o \
    $(SRCDIR)/management/gpu/gpu.o \
    $(SRCDIR)/management/gpu/archs/eard.o \
    $(SRCDIR)/management/gpu/archs/dummy.o \
    $(SRCDIR)/management/gpu/archs/nvml.o \
    $(SRCDIR)/management/gpu/archs/oneapi.o \
    $(SRCDIR)/management/gpu/archs/rsmi.o \
    $(SRCDIR)/management/management.o

######## RULES

all: all.components $(root_BINS)

post: test

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

libmgt.a: Makefile $(root_OBJS)
	$(AR) rvs $@ $(root_OBJS)

test: management.c
	$(CC) -I $(SRCDIR) -DTEST=1 -fPIC -o $@ $^ libmgt.a ../metrics/libmetrics.a ../daemon/local_api/libeard.a ../common/libcommon.a -lm -lpthread -ldl

######## OPTIONS

install: install.components;

devel.install: ;

%.install: ;

clean: clean.components stat.gclean rclean
	rm -f test

depend: depend.components

depend-clean: depend-clean.components

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'cpufreq' $* || exit
	@ $(MAKE) -C 'cpupow'  $* || exit
	@ $(MAKE) -C 'imcfreq' $* || exit
	@ $(MAKE) -C 'gpu'     $* || exit

######## EXPORTS

include $(SRCDIR)/Makefile.extra
