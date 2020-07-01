CFLAGS = $(CC_FLAGS) -fPIC -I$(SRCDIR) $(CONSTANTS)

######## FILES

type_OBJS = $(patsubst %.c, %.o, $(shell find -maxdepth 1 -type f -name '*.c'))

######## RULES

all: all.components $(type_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: install.components

%.install: %.install.components;

clean: clean.components rclean;

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'configuration' $* || exit

######## DEPENDENCIES

depend: depend.components

depend-clean: depend-clean.components

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
