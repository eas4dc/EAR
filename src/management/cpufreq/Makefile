CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC -Wall

######## FILES

root_OBJS = \
    cpufreq.o \
	cpufreq_base.o \
    frequency.o \
	governor.o \
	priority.o

######## RULES

all: all.components $(root_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

test_prio: priority.o
	$(CC) $(CFLAGS) -o $@ $< ./archs/prio_dummy.o ./archs/prio_isst.o ./archs/prio_eard.o ../../daemon/lib  $(SRCDIR)/daemon/local_app/libeard.a $(SRCDIR)/metrics/libmetrics.a $(SRCDIR)/common/libcommon.a -lpthread

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
