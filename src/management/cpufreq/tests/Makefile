SRCDIR   = ../../..
CC       = /hpc/base/ctt/packages/compiler/gnu/9.1.0/bin/gcc
CC_FLAGS = -Wall -fPIC -I ../../..
DEPS     = $(SRCDIR)/management/libmgt.a $(SRCDIR)/metrics/libmetrics.a $(SRCDIR)/common/libcommon.a

######## RULES

all: $(arch_OBJS)
	$(CC) $(CC_FLAGS) -DCONNECTOR=1 -o test_conn connector_test.c $(DEPS) -lpthread -lm
	$(CC) $(CC_FLAGS) -o test test.c $(DEPS) -lpthread
	

######## OPTIONS

install: ;

clean: rclean;
	rm test
	rm test_conn

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
