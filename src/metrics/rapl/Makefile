CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

flop_OBJS = rapl.o

######## RULES

all: all.components

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'archs' $* || exit

######## OPTIONS

install: ;

%.install: ;

clean: rclean clean.components;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
