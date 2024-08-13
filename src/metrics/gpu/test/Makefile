CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC -Wall

######## RULES

all:

post: libtest.so gpuprof_test

libtest.so: test.c
	$(CC) -I $(SRCDIR) -fPIC --shared -o $@ $^ ../../libmetrics.a ../../../common/libcommon.a -lm -lpthread -ldl

gpuprof_test: gpuprof_test.c ../gpuprof.h ../../common/apis.h ../../libmetrics.a ../../../common/libcommon.a
	$(CC) $(CFLAGS) -o $@ gpuprof_test.c ../../libmetrics.a ../../../common/libcommon.a -lpthread -lm -ldl

######## OPTIONS

install: ;

clean: rclean; 

######## EXPORTS

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
