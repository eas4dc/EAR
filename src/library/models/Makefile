######## FILES

modl_BINS = \
    basic_model.so \
    avx512_model.so \
		cpu_power_model_default.so \
		cpu_power_model_gflops.so \
		cpu_power_model_dummy.so

modl_OBJS = models.o cpu_power_model.o 
modl_PATH = $(DESTDIR)/lib/plugins/models
modl_PERM = 0775
modl_DEPS = \
    $(SRCDIR)/common/libcommon.a


########

.PRECIOUS: %.o

all: $(modl_OBJS) $(modl_BINS)

%.o: %.c %.h
	$(CC) $(CC_FLAGS) $(CFLAGS) $(LIB_CFLAGS) -c $<

%.o: %.c
	$(CC) $(CC_FLAGS) $(CFLAGS) $(LIB_CFLAGS) -c $<

%.so: %.o $(modl_DEPS)
	$(CC) $(CC_FLAGS) -shared -o $@ $^ 

######## OPTIONS 

install: modl.ginstall;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
