CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) -O2 -fPIC

######## FILES

modl_OBJS = \
    models.o
modl_BINS = \
    basic_model.so \
    avx512_model.so
modl_PATH = $(DESTDIR)/lib/plugins/models
modl_PERM = 0775

deps= $(SRCDIR)/daemon/shared_configuration.o \
			$(SRCDIR)/common/libcommon.a

########

all: $(modl_OBJS) $(modl_BINS)

%.o: %.c %.h
	$(MPICC) $(CFLAGS) -c $<


%.so: %.o
	$(CC) -shared -o $@ $< 

######## OPTIONS 

install: modl.ginstall;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
