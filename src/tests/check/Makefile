CFLAGS = $(CC_FLAGS) -I$(SRCDIR) $(SLURM_CFLAGS)

######## INSTALLATION

chck_BINS = \
    ear_types           \
    cpu_examinable      \
    cpu_aperf           \
    cpu_uncores         \
    cpu_uncores_all     \
    gsl_version         \
    module_acpi_cpufreq \
    module_ipmi_devintf
ifdef PAPI_BASE
chck_BINS += \
    papi_init           \
    papi_version        \
    papi_comp_available \
    papi_comp_enabled
endif
ifdef SLURM_BASE
#   chck_BINS += slurm_version
endif

######## FILES

metr_OBJS = $(SRCDIR)/metrics/libmetrics.a

comm_OBJS = $(SRCDIR)/common/libcommon.a -lm

chck_OBJS = check.o
 
######## RULES

.PHONY: check

all: $(chck_OBJS) $(chck_BINS)

check.o: check.c check.h
	$(CC) $(CFLAGS) -c $<

papi_init: papi_init.c
	$(CC) $(CFLAGS) $(PAPI_CFLAGS) -o $@ $< $(PAPI_LDFLAGS)

papi_version: papi_version.c
	$(CC) $(CFLAGS) $(PAPI_CFLAGS) -o $@ $<

papi_comp_available: papi_comp_available.c
	$(CC) $(CFLAGS) $(PAPI_CFLAGS) -o $@ $< $(PAPI_LDFLAGS)

papi_comp_enabled: papi_comp_enabled.c
	$(CC) $(CFLAGS) $(PAPI_CFLAGS) -o $@ $< $(PAPI_LDFLAGS)

module_acpi_cpufreq: module_acpi_cpufreq.c $(chck_OBJS) $(comm_OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(chck_OBJS) $(comm_OBJS)

module_ipmi_devintf: module_ipmi_devintf.c $(chck_OBJS) $(comm_OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(chck_OBJS) $(comm_OBJS)

cpu_examinable: cpu_examinable.c
	$(CC) $(CFLAGS) -o $@ $< $(comm_OBJS)

cpu_aperf: cpu_aperf.c
	$(CC) $(CFLAGS) -o $@ $< $(comm_OBJS)

cpu_uncores: cpu_uncores.c
	$(CC) $(CFLAGS) -o $@ $< $(comm_OBJS) $(metr_OBJS)

cpu_uncores_all: cpu_uncores_all.c
	$(CC) $(CFLAGS) -o $@ $< $(comm_OBJS) $(metr_OBJS)

gsl_version: gsl_version.c
	$(CC) $(CFLAGS) $(GSL_CFLAGS) -o $@ $<

slurm_version: slurm_version.c	
	$(CC) $(CFLAGS) $(SLURM_CFLAGS) -o $@ $<

ear_types: ear_types.c 
	$(CC) $(CFLAGS) -o $@ $<

######## OPTIONS

check:
	@./check.sh

install: ;

clean: chck.gclean rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
