CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

arch_OBJS = \
    frequency.o \
    cpupower.o

######## RULES

all: $(arch_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) $(DEFS) -c $<

######## OPTIONS

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
