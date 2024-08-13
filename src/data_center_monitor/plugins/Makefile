CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC -shared

######## RULES

plug_PATH = $(DESTDIR)/lib/plugins/monitoring
plug_PERM = 0775
plug_BINS = \
    conf.so \
    dummy.so \
    eardcon.so \
    eard_serialization.so \
    keyboard.so \
    management.so \
    management_viewer.so \
    metrics.so \
    metrics_viewer.so \
    periodic_metrics.so \
    test_cpufreq.so \
    test_gpu.so \
    nodesensors.so \
    nodesensors_report.so \
    nodesensors_alerts.so

ifdef CUDA_BASE 
ifdef KERNELS
plug_BINS += kernel_cuda.so
endif
endif

ifdef OPENCL_BASE
plug_BINS += kernel_cl.so
endif

all: $(plug_BINS) dummy.so

%.so: %.c
	$(CC) $(CFLAGS)  -o $@ $^

conf.so:conf.c
	$(CC)  $(CFLAGS)  -o $@ $^ $(call whole,$(SRCDIR)/common/libcommon.a)

eardcon.so: eardcon.c $(SRCDIR)/daemon/local_api/libeard.a
	$(CC) $(CFLAGS)  -o $@ $^

management.so: management.c
	$(CC)  $(CFLAGS)  -o $@ $^ $(call whole,$(SRCDIR)/management/libmgt.a)

metrics.so: metrics.c
	$(CC) $(CFLAGS)  -o $@ $^ $(call whole,$(SRCDIR)/metrics/libmetrics.a) $(SRCDIR)/daemon/local_api/libeard.a -lm

periodic_metrics.so: periodic_metrics.c
	$(CC) $(CFLAGS)  -o $@ $^ $(SRCDIR)/report/report.o

dummy.so: dummy.c
	$(CC) $(CFLAGS)  -o $@ $^

nodesensors.so: nodesensors.c
	$(CC) $(CFLAGS)  -o $@ $^ $(SRCDIR)/common/libcommon.a $(SRCDIR)/metrics/libmetrics.a -lm 

nodesensors_report.so: nodesensors_report.c
	$(CC) $(CFLAGS) -o $@ $^ $(SRCDIR)/common/libcommon.a $(SRCDIR)/report/report.o

nodesensors_alerts.so: nodesensors_alerts.c
	$(CC) $(CFLAGS) -o $@ $^ $(SRCDIR)/common/libcommon.a 

kernel_cuda.so: kernel_cuda.c kernel_cuda_ptx.cu
	$(CUDA_BASE)/bin/nvcc  $(CFLAGS)  --compiler-options '-fPIC'  -o $@ $^

kernel_cl.so: kernel_cl.c
	$(CC) $(CFLAGS) $(OPENCL_CFLAGS) -D CL_TARGET_OPENCL_VERSION=100  -o $@ $^ $(OPENCL_LDFLAGS)

######## OPTIONS

install: plug.ginstall;

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
