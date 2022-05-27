CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC -Wall -g

######## FILES

root_OBJS = \
    cpufreq.o \
    frequency.o \
	governor.o

######## RULES

all: all.components $(root_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'archs'    $* || exit
	@ $(MAKE) -C 'drivers'  $* || exit

######## OPTIONS

install: install.components

%.install: ;

clean: rclean clean.components

depend: depend.components

depend-clean: depend-clean.components

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
