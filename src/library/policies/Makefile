######## FILES

poli_OBJS = \
       policy.o
modl_BINS = \
       min_time.so \
       min_energy.so \
       min_time_no_models.so \
       min_energy_no_models.so \
			 min_time_avx512.so \
			 min_energy_avx512.so \
			 min_energy_auto.so \
       monitoring.so
modl_PATH = $(DESTDIR)/lib/plugins/policies
modl_PERM = 0775

########

all: $(poli_OBJS) $(modl_BINS)

%.o: %.c 
	$(MPICC) $(MPICC_FLAGS)  $(CFLAGS) -c $<

%.so: %.o 
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ 	

######## OPTIONS 

install: modl.ginstall;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
