CFLAGS = $(CC_FLAGS) -fPIC -I$(SRCDIR) -DARCH_$(ARCH)=1

######## FILES

libc_OBJS = \
	architecture.o \
	bithack.o \
	cpuid.o \
	hardware_info.o \
	mrs.o \
	topology.o \
	topology_asm.o \
	topology_tdp.o

######## RULES

all: $(libc_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: ;

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
