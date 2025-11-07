CFLAGS = $(CC_FLAGS) -fPIC -I$(SRCDIR) $(CONSTANTS) 

######## FILES

comm_BINS = libmetrics-common.a

comm_OBJS = \
    apis.o \
    cupti.o \
    file.o \
    hsmp.o \
    hsmp_driver.o \
    hsmp_esmi.o \
    hsmp_pci.o \
    hwmon.o \
    hwmon_old.o \
    ipmi.o \
    ipmi_driver.o \
    ipmi_driver_frusdr.o \
    ipmi_driver_parsing.o \
    isst.o \
    likwid.o \
    msr.o \
    nvml.o \
    offsets.o \
    omsr.o \
    oneapi.o \
    pci.o \
    perf.o \
    pstate.o \
    redfish.o \
    rsmi.o \
    cray_pm_counters.o

######## RULES

all: $(comm_BINS)

hsmp_esmi.o: hsmp_esmi.c hsmp_esmi.h
	$(CC) $(CFLAGS) $(ESMI_CFLAGS) -c $<

nvml.o: nvml.c nvml.h
	$(CC) $(CFLAGS) $(CUDA_CFLAGS) -c $<

cupti.o: cupti.c cupti.h
	$(CC) $(CFLAGS) $(CUDA_CFLAGS) $(CUPTI_CFLAGS) -c $<

likwid.o: likwid.c likwid.h
	$(CC) $(CFLAGS) $(LIKWID_CFLAGS) -c $<

rsmi.o: rsmi.c rsmi.h
	$(CC) $(CFLAGS) $(RSMI_CFLAGS) -c $<

oneapi.o: oneapi.c oneapi.h
	$(CC) $(CFLAGS) $(ONEAPI_CFLAGS) -c $<

redfish.o: redfish.c redfish.h
	$(CC) $(CFLAGS) $(REDFISH_CFLAGS) -c $<

libmetrics-common.a: Makefile $(comm_OBJS)
	$(AR) rvs $@ $(comm_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

post: nvml_test

nvml_test: nvml.c nvml.h
	$(CC) $(CFLAGS) $(CUDA_CFLAGS) -DTEST=1 -o $@ nvml.c ../../common/libcommon.a -ldl -lpthread

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'cpu' $* || exit

######## OPTIONS

install: ;

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
