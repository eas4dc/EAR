CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

freq_OBJS = \
	cpu.o \
	cpufreq_base.o

######## RULES

all: all.components $(freq_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'cpu' $* || exit

######## OPTIONS

install: ;

%.install: ;

clean: rclean clean.components;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
