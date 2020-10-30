CFLAGS = $(CC_FLAGS) -fPIC -I$(SRCDIR) $(CONSTANTS)

######## FILES

libc_OBJS = \
	architecture.o \
	cpuid.o \
	cpupower.o \
	frequency.o \
	hardware_info.o \
	topology.o \
	topology_frequency.o \

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
