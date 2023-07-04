CFLAGS = $(CC_FLAGS) -fPIC -I$(SRCDIR) $(CONSTANTS) 

######## FILES

comm_OBJS = \
    apis.o \
    cupti.o \
    file.o \
    hsmp.o \
    hwmon.o \
    isst.o \
    likwid.o \
    msr.o \
    nvml.o \
    omsr.o \
    oneapi.o \
    pci.o \
    perf.o \
    popen.o \
    pstate.o \
    redfish.o \
    rsmi.o

######## RULES

all: $(comm_OBJS)

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

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

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
