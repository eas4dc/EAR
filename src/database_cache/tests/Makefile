SRCDIR   = ../..
CC       = /hpc/base/ctt/packages/compiler/gnu/9.1.0/bin/gcc
CC       = /opt/rh/devtoolset-7/root/bin/gcc
CC_FLAGS = -Wall -fPIC -O2 -I $(SRCDIR)

DEPS = \
    $(SRCDIR)/database_cache/eardbd_api.o \
    $(SRCDIR)/common/libcommon.a

######## RULES

all:
	$(CC) $(CC_FLAGS) -o test_status test_status.c $(DEPS)
	$(CC) $(CC_FLAGS) -o test_types test_types.c $(DEPS)

######## OPTIONS

install: ;

clean: rclean;
	rm test_status test_types

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
