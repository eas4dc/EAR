CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC -Wall

######## RULES

all:

post: libtest.so

libtest.so: test.c
	$(CC) -I $(SRCDIR) -fPIC --shared -o $@ $^ ../../libmetrics.a ../../../common/libcommon.a -lm -lpthread -ldl

######## OPTIONS

install: ;

clean: rclean; 

######## EXPORTS

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
