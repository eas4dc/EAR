CFLAGS = $(CC_FLAGS) -fPIC -I$(SRCDIR) $(CONSTANTS)

######## FILES

libc_OBJS = frequency.o cpupower.o cpuid.o hardware_info.o architecture.o

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
