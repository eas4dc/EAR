CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC $(CONSTANTS) -Wall

######## FILES

powercap_OBJS = \
    powercap_status.o \
    powercap_mgt.o \
    powercap.o

powercap_BINS = \
    inm.so \
    dvfs.so \
    cpu_generic.so \
    gpu_dummy.so \
    gpu.so \
    gpu_dvfs.so

powercap_PATH = $(DESTDIR)/lib/plugins/powercap
powercap_PERM = 0775
gpu_DEPS      =
gpu_dvfs_DEPS =
dvfs_DEPS     = $(SRCDIR)/metrics/libmetrics.a
generic_DEPS  = $(SRCDIR)/management/libmgt.a $(SRCDIR)/metrics/libmetrics.a $(SRCDIR)/common/libcommon.a
inm_DEPS      = $(SRCDIR)/common/libcommon.a

######## RULES

all:$(powercap_OBJS)  $(powercap_BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.so: %.o 
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ 

dvfs.so:dvfs.o $(dvfs_DEPS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(generic_DEPS)

cpu_generic.so:cpu_generic.o $(generic_DEPS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(generic_DEPS)

gpu.so:gpu.o $(gpu_DEPS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(gpu_DEPS)

gpu_dvfs.so:gpu_dvfs.o $(gpu_dvfs_DEPS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(gpu_dvfs_DEPS)

inm.so:inm.o $(inm_DEPS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(inm_DEPS)

powercap.o: powercap.c powercap.h
	$(CC) $(CFLAGS) -c $<

powercap_status.o:powercap_status.c powercap_status.h powercap.h
	$(CC) $(CFLAGS) -fPIC -c $< 

powercap_mgt.o: powercap_mgt.c powercap_mgt.h
	$(CC) $(CFLAGS) -c $< 


######## OPTIONS

install: powercap.ginstall;
%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
