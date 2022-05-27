CFLAGS  = -O2 -fPIC -I$(SRCDIR)
LDFLAGS = -ldl

LOAD_CFLAGS = $(CFLAGS)

######## FILES

load_OBJS = \
    loader.o \
    module_common.o \
    module_cuda.o \
    module_openmp.o \
    module_default.o \
    module_mpi.o \
    module_mpic.o \
    module_mpif.o

load_BINS = libearld.so
load_DEPS = $(SRCDIR)/common/libcommon.a
load_PATH = $(DESTDIR)/lib
load_PERM = 0775

######## RULES

all: $(load_BINS)

%.o: %.c %.h
	$(CC) $(MPI_CFLAGS) $(CC_FLAGS) $(LOAD_CFLAGS) -c $<


libearld.so: $(load_OBJS) $(load_DEPS)
	$(CC) $(CC_FLAGS) -shared -o $@ $^ $(LDFLAGS)

######## OPTIONS 

install: load.ginstall;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
