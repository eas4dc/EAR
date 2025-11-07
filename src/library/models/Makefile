######## FILES

modl_BINS = \
						energy_models/basic_model.so \
						energy_models/avx512_model.so \
						energy_models/imc_linear_model.so \
						energy_models/cpu_dummy_model.so \
						energy_models/spr_imc_perc_model.so \
						cpu_power_model_default.so \
						cpu_power_model_gflops.so \
						cpu_power_model_dummy.so

ifeq ($(FEAT_GPUS), 1)
modl_BINS += \
	     energy_models/gpu_dummy_model.so
endif

modl_OBJS = cpu_power_model.o energy_model.o 
modl_PATH = $(DESTDIR)/lib/plugins/models
modl_PERM = 0775
modl_DEPS = $(SRCDIR)/common/libcommon.a

# Additional dependencies for energy models plug-ins
enermodl_DEPS = energy_models/common.o

######## RULES

.PRECIOUS: $(modl_OBJS)

.PHONY: clean install

all: $(modl_OBJS) $(modl_BINS)

%.o: %.c %.h
	$(CC) $(CC_FLAGS) $(CFLAGS) $(LIB_CFLAGS) -o $@ -c $<

%.o: %.c
	$(CC) $(CC_FLAGS) $(CFLAGS) $(LIB_CFLAGS) -o $@ -c $<

# Implicit rule for energy model plug-ins
%_model.so: %_model.o $(enermodl_DEPS) $(modl_DEPS)
	$(CC) $(CC_FLAGS) -shared -o $@ $^

cpu_power_model_%.so: cpu_power_model_%.o $(modl_DEPS)
	$(CC) $(CC_FLAGS) -shared -o $@ $^

######## OPTIONS 

install: modl.ginstall;

clean: modl.gclean
	rm -rf *.o *.so

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
