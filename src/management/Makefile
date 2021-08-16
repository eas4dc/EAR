######## FILES

root_BINS = libmgt.a

root_OBJS = \
    $(SRCDIR)/management/cpufreq/cpufreq.o \
    $(SRCDIR)/management/cpufreq/governor.o \
    $(SRCDIR)/management/cpufreq/frequency.o \
    $(SRCDIR)/management/cpufreq/archs/amd17.o \
    $(SRCDIR)/management/cpufreq/archs/default.o \
    $(SRCDIR)/management/cpufreq/drivers/linux_cpufreq.o \
    $(SRCDIR)/management/imcfreq/imcfreq.o \
    $(SRCDIR)/management/imcfreq/archs/amd17.o \
    $(SRCDIR)/management/imcfreq/archs/dummy.o \
    $(SRCDIR)/management/imcfreq/archs/eard.o \
    $(SRCDIR)/management/imcfreq/archs/intel63.o \
    $(SRCDIR)/management/gpu/gpu.o \
    $(SRCDIR)/management/gpu/archs/nvml.o \
    $(SRCDIR)/management/gpu/archs/dummy.o
    

######## RULES

all: all.components $(root_BINS)

libmgt.a: Makefile $(root_OBJS)
	$(AR) rvs $@ $(root_OBJS)

######## OPTIONS

install: install.components;

devel.install: ;

%.install: ;

clean: clean.components stat.gclean;

depend: depend.components

depend-clean: depend-clean.components

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'cpufreq' $* || exit
	@ $(MAKE) -C 'imcfreq' $* || exit
	@ $(MAKE) -C 'gpu'     $* || exit

######## EXPORTS

include $(SRCDIR)/Makefile.extra
