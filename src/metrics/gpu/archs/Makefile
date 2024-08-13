CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

gpu_OBJS = \
    cupti.o \
    dcgmi.o \
    dummy.o \
    eard.o \
    nvml.o \
    oneapi.o \
    rsmi.o \
    gpuprof_dcgmi.o \
    gpuprof_dummy.o \
    gpuprof_nvml.o

######## RULES

all: $(gpu_OBJS)

nvml.o: nvml.c nvml.h
	$(CC) $(CFLAGS) $(CUDA_CFLAGS) -c $<

cupti.o: cupti.c cupti.h
	$(CC) $(CFLAGS) $(CUPTI_CFLAGS) -c $<

oneapi.o: oneapi.c oneapi.h
	$(CC) $(CFLAGS) $(ONEAPI_CFLAGS) -c $<

rsmi.o: rsmi.c rsmi.h
	$(CC) $(CFLAGS) $(RSMI_CFLAGS) -c $<

gpuprof_nvml.o: gpuprof_nvml.c gpuprof_nvml.h
	$(CC) $(CFLAGS) $(CUDA_CFLAGS) -c gpuprof_nvml.c

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: ;

clean: rclean;

######## EXPORTS

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
