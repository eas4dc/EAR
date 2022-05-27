SRCDIR   = ../../..
CC       = /opt/rh/devtoolset-7/root/bin/gcc
CC       = /hpc/base/ctt/packages/compiler/gnu/9.1.0/bin/gcc
CC_FLAGS = -Wall -fPIC -I ../../.. -g

######## FILES

DEPS = \
    $(SRCDIR)/management/libmgt.a \
    $(SRCDIR)/metrics/libmetrics.a \
    $(SRCDIR)/daemon/local_api/libeard.a \
    $(SRCDIR)/common/libcommon.a

######## RULES

all: $(arch_OBJS)
	$(CC) $(CC_FLAGS) -o test test.c $(DEPS) -lpthread -lm

######## OPTIONS

install: ;

clean: rclean;
	rm test

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra