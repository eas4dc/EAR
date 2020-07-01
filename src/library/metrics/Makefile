all: metrics.o

metrics.o: metrics.c metrics.h
	$(CC) $(CC_FLAGS) $(CFLAGS) -c metrics.c

######## OPTIONS 

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
