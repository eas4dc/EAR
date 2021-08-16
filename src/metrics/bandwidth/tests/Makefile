CC     = /hpc/base/ctt/packages/compiler/gnu/9.1.0/bin/gcc
SRCDIR = ../../..

######## RULES

all: api.c
	$(CC) -Wall -fPIC -I $(SRCDIR) -o test $< $(SRCDIR)/metrics/libmetrics.a $(SRCDIR)/common/libcommon.a -lpthread -lm

######## OPTIONS

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
