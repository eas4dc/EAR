CFLAGS = -fPIC -O2 -I$(SRCDIR)
LDFLAGS =

# INSTALLATION

pbsutils_BINS = libpbs_hook_utils.so

pbsutils_PATH = $(DESTDIR)/lib/pbs
pbsutils_PERM = 0775

eardrapi_BINS = libeard_rapi.so

eardrapi_PATH = $(DESTDIR)/lib/pbs
eardrapi_PERM = 0775

# FILES

eardrapi_OBJS = \
			   $(SRCDIR)/daemon/remote_api/eard_rapi_internals.o \
			   $(SRCDIR)/common/libcommon.a

pbsutils_OBJS = \
				$(SRCDIR)/daemon/shared_configuration.o \
			   	$(SRCDIR)/common/libcommon.a

pbsutils_SRCS = \
			   pbs_hook_utils.c

pbsutils_HDRS = \
			   pbs_hook_utils.h

# RULES

all: $(pbsutils_BINS) $(eardrapi_BINS) $(libhardware_BINS)

libeard_rapi.so: $(eardrapi_OBJS)
	$(CC) $(CC_FLAGS) $(CFLAGS) $(LDFLAGS) -shared -o $@ $(eardrapi_OBJS)

libpbs_hook_utils.so: $(pbsutils_SRCS) $(pbsutils_HDRS)
	$(CC) $(CC_FLAGS) $(CFLAGS) $(LDFLAGS) -shared -o $@ $(pbsutils_SRCS) $(pbsutils_OBJS)

# OPTIONS

install: eardrapi.ginstall pbsutils.ginstall;

%.install: ;

clean: eardrapi.gclean pbsutils.gclean;

# DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
