include ./Makefile.files

CFLAGS = -fPIC -I$(SRCDIR) $(CONSTANTS) -O2

######## FILES

# The list of libc_OBJS is in Makefile.files
libc_BINS = libcommon.a
comm_OBJS = $(patsubst %.c, %.o, $(shell find -maxdepth 1 -type f -name '*.c'))
incl_BINS = states.h
incl_PATH = $(DESTDIR)/include
incl_PERM = 0660

######## RULES

all: all.components $(comm_OBJS) $(libc_BINS)

%.o: %.c %.h
	$(CC) $(CC_FLAGS) $(CFLAGS) -c $<

libcommon.a: $(libc_OBJS)
	$(AR) rvs $@ $(libc_OBJS)

######## OPTIONS

install: install.components;

devel.install: devel.install.components incl.ginstall;

%.install: %.install.components;

clean: clean.components comm.gclean rclean;

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'config'		$* || exit
	@ $(MAKE) -C 'database' 	$* || exit
	@ $(MAKE) -C 'hardware' 	$* || exit
	@ $(MAKE) -C 'messaging' 	$* || exit
	@ $(MAKE) -C 'system'   	$* || exit
	@ $(MAKE) -C 'types'    	$* || exit
	@ $(MAKE) -C 'utils'		$* || exit

######## DEPENDENCIES

depend: depend.components

depend-clean: depend-clean.components

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
