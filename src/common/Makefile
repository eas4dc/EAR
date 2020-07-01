CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC $(CONSTANTS) -O2

######## INSTALLATION

libc_BINS = libcommon.a
libc_OBJS = $(shell find ./ -type f -name '*.o')
comm_OBJS = $(patsubst %.c, %.o, $(shell find -maxdepth 1 -type f -name '*.c'))
incl_BINS = states.h
incl_PATH = $(DESTDIR)/include
incl_PERM = 0660

######## FILES

######## RULES

all: all.components $(libc_BINS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

libcommon.a: $(comm_OBJS)
	$(AR) rvs $@ $(libc_OBJS)

######## OPTIONS

install: install.components;

devel.install: devel.install.components incl.ginstall;

%.install: %.install.components;

clean: clean.components libc.gclean rclean;

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'types'    $* || exit
	@ $(MAKE) -C 'system'   $* || exit
	@ $(MAKE) -C 'hardware' $* || exit
	@ $(MAKE) -C 'database' $* || exit

######## DEPENDENCIES

depend: depend.components

depend-clean: depend-clean.components

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
