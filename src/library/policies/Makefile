CFLAGS = $(CC_FLAGS) $(MPI_CFLAGS) -I$(SRCDIR) -O2 -fPIC

######## FILES

poli_OBJS = \
    policy.o 

modl_BINS = monitoring.so 

modl_PATH = $(DESTDIR)/lib/plugins/policies
modl_PERM = 0775

eard_modl_BINS =  monitoring_eard.so 

eard_modl_PATH = $(DESTDIR)/lib/plugins/eard_policies
eard_modl_PERM = 0775

deps= $(SRCDIR)/common/libcommon.a 

########

all: $(poli_OBJS) $(modl_BINS) $(eard_modl_BINS)

min_time_eard.o: min_time_eard.c
	$(CC) $(CC_FLAGS)  $(CFLAGS) -c $<

min_energy_eard.o:min_energy_eard.c
	$(CC) $(CC_FLAGS)  $(CFLAGS) -c $<
monitoring_eard.o:monitoring_eard.c
	$(CC) $(CC_FLAGS)  $(CFLAGS) -c $<

%.o: %.c 
	$(CC) $(CFLAGS) -c $<

%.so: %.o $(deps)
	$(CC) -shared -o $@ $< 

min_energy_eard.so:min_energy_eard.o $(SRCDIR)/common/libcommon.a
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@   $(SRCDIR)/common/libcommon.a
min_time_eard.so:min_time_eard.o $(SRCDIR)/common/libcommon.a
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@   $(SRCDIR)/common/libcommon.a
monitoring_eard.so:monitoring_eard.o $(SRCDIR)/common/libcommon.a
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@   $(SRCDIR)/common/libcommon.a


min_time_mpi_info.so: min_time_mpi_info.o $(SRCDIR)/common/system/time.o
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(SRCDIR)/common/system/time.o

monitoring_ext.so:monitoring_ext.o $(SRCDIR)/common/system/time.o	
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(SRCDIR)/common/system/time.o
monitoring_ext_notime.so:monitoring_ext_notime.o $(SRCDIR)/common/system/time.o	
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(SRCDIR)/common/system/time.o

######## OPTIONS 

install: modl.ginstall eard_modl.ginstall;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
