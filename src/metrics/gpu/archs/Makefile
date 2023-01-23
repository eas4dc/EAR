CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC -Wall

######## FILES

gpu_OBJS = \
    cupti.o \
    dummy.o \
    eard.o \
    nvml.o \
    oneapi.o

######## RULES

all: $(gpu_OBJS)

nvml.o: nvml.c nvml.h
	$(CC) $(CFLAGS) $(CUDA_CFLAGS) -c $<

cupti.o: cupti.c cupti.h
	$(CC) $(CFLAGS) $(CUPTI_CFLAGS) -c $<

oneapi.o: oneapi.c oneapi.h
	$(CC) $(CFLAGS) $(ONEAPI_CFLAGS) -c $<

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: ;

clean: rclean;

######## EXPORTS

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
