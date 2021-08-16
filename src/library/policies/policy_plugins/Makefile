ifeq ($(FEAT_GPU_OPT), 1)
GPU_OPT_FLAG = -DGPU_OPT=1
endif

######## FILES

poli_BINS = \
    min_energy.so \
    min_time.so  \
    monitoring.so

ifdef CUDA_BASE
poli_BINS += \
    gpu_monitoring.so
ifeq ($(FEAT_GPU_OPT),1)
poli_BINS += \
    gpu_min_energy.so \
    gpu_min_time.so
endif
endif

poli_PERM = 0775
poli_PATH = $(DESTDIR)/lib/plugins/policies
poli_DEPS = \
    $(SRCDIR)/daemon/powercap/powercap_status.o \
    $(SRCDIR)/common/libcommon.a

########

.PRECIOUS: %.o

all: $(poli_BINS)
 
%.o: %.c 
	$(CC) $(CC_FLAGS) $(CFLAGS) $(LIB_CFLAGS) $(GPU_OPT_FLAG) -c $<

%.so: %.o $(poli_DEPS)
	$(CC) $(CC_FLAGS) -shared -fPIC -o $@ $^ 

######## OPTIONS 

install: poli.ginstall;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
