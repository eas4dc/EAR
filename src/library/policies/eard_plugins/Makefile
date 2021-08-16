######## FILES

eard_BINS = \
    min_energy_eard.so \
    min_time_eard.so \
    monitoring_eard.so

eard_PERM = 0775
eard_PATH = $(DESTDIR)/lib/plugins/eard_policies
eard_DEPS = $(SRCDIR)/common/libcommon.a

########

.PRECIOUS: %.o

all: $(eard_BINS)
 
%.o: %.c 
	$(CC) $(CC_FLAGS) $(CFLAGS) $(LIB_CFLAGS) -c $<

%_eard.so: %_eard.o $(eard_DEPS)
	$(CC) $(CC_FLAGS) -shared -fPIC -o $@ $^

######## OPTIONS 

install: eard.ginstall;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
