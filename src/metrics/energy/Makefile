CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

ener_BINS = \
    energy_cpu.o \
    energy_gpu.o \
    energy_node.o \
    energy_node_lib.o
ener_DEPS = \
    $(SRCDIR)/common/libcommon.a

######## RULES

all:  $(ener_BINS) all.components

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'cpu' $* || exit
	@ $(MAKE) -C 'gpu' $* || exit
	@ $(MAKE) -C 'node' $* || exit

######## OPTIONS

install: install.components;

%.install: ;

clean: rclean clean.components;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
