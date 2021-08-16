CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC $(CONSTANTS) -Wall

######## FILES

powercap_OBJS = \
		powercap_status.o \
		powercap_mgt.o \
  	powercap.o 

powercap_BINS = \
    inm.so \
		dvfs.so 

ifdef CUDA_BASE
powercap_BINS += \
		gpu_dummy.so \
	  gpu.so \
		gpu_dvfs.so
endif

powercap_PATH = $(DESTDIR)/lib/plugins/powercap
powercap_PERM = 0775

gpu_DEPS = 

gpu_dvfs_DEPS = 

dvfs_DEPS = 

inm_DEPS = $(SRCDIR)/common/libcommon.a

######## RULES

all:$(powercap_OBJS)  $(powercap_BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<
dvfs.so:dvfs.o $(dvfs_DEPS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(dvfs_DEPS)

gpu.so:gpu.o $(gpu_DEPS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(gpu_DEPS)

gpu_dvfs.so:gpu_dvfs.o $(gpu_dvfs_DEPS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(gpu_dvfs_DEPS)

inm.so:inm.o $(inm_DEPS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(inm_DEPS)


%.so: %.o 
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ 

powercap.o: powercap.c powercap.h
	$(CC) $(CFLAGS) -c $<

powercap_status.o:powercap_status.c powercap_status.h powercap.h
	$(CC) $(CFLAGS) -fPIC -c $< 
powercap_mgt.o:powercap_mgt.c powercap_mgt.h
	$(CC) $(CFLAGS) -c $< 


######## OPTIONS

install: powercap.ginstall;
%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
