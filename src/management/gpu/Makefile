CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC -Wall

######## FILES

gpu_BINS = gpu.o

######## RULES

all: $(gpu_BINS) all.components

%.o: %.c %.h
	$(CC) $(CFLAGS) $(gpu_DEFS) -c $<

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'archs' $* || exit

######## OPTIONS

install: install.components

%.install: ;

clean: rclean clean.components

depend: depend.components

depend-clean: depend-clean.components

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
