CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

cycl_OBJS = cpi.o

######## RULES

all: all.components $(cycl_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'cpu' $* || exit

######## OPTIONS

install: ;

%.install: ;

clean: rclean clean.components;

depend: depend.components

depend-clean: depend-clean.components

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
