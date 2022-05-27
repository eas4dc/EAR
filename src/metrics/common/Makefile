CFLAGS = $(CC_FLAGS) -fPIC -I$(SRCDIR) $(CONSTANTS) 

######## FILES

comm_OBJS = \
    apis.o \
    pstate.o \
    hsmp.o \
    hwmon.o \
    likwid.o \
    msr.o \
    omsr.o \
    pci.o \
    perf.o

######## RULES

all: $(comm_OBJS)

likwid.o: likwid.c likwid.h
	$(CC) $(CFLAGS) $(LIKWID_CFLAGS) -c $<

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'cpu' $* || exit

######## OPTIONS

install: ;

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra