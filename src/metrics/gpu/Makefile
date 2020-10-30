CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

gpu_BINS = \
    gpu.o

######## RULES

all: $(gpu_BINS) all.components

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'gpu' $* || exit

######## OPTIONS

install: install.components;

%.install: ;

clean: rclean clean.components;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
