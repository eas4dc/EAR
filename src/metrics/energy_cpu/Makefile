CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

energy_cpu_OBJS = energy_cpu.o

######## RULES

all: all.components $(energy_cpu_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<
#	$(CC) $(CFLAGS) -o energy_cpu.o $(SRCDIR)/metrics/libmetrics.a $(SRCDIR)/common/libcommon.a

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'archs' $* || exit

######## OPTIONS

install: install.components;

%.install: ;

clean: rclean clean.components;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
