CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC -Wall

######## FILES

gpu_OBJS = \
    dummy.o \
    nvml.o

ifdef CUDA_BASE
gpu_DEFS = -DCUDA_BASE=\"$(CUDA_BASE)\"
endif

######## RULES

all: $(gpu_OBJS)

nvml.o: nvml.c nvml.h
	$(CC) $(CFLAGS) -I$(CUDA_BASE)/include $(gpu_DEFS) -c $<

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: ;

clean: rclean;

######## EXPORTS

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
