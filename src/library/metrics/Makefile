CFLAGS = $(CC_FLAGS) -I$(SRCDIR) $(PAPI_CFLAGS) -O2 -fPIC

########

all: \
        gpu.o \
        metrics.o 

%.o: %.c %.h
	$(MPICC) $(CFLAGS) -c $<

######## OPTIONS 

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
