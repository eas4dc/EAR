CFLAGS = -fPIC -I$(SRCDIR) $(CC_FLAGS) $(MPI_CFLAGS)

######## FILES

load_OBJS = \
    loader.o \
    module_common.o \
    module_cuda.o \
    module_mpi.o \
    module_openmp.o \
    module_oneapi.o \
    module_default.o

ifeq ($(FEAT_MPI_LOADER), 1)
load_OBJS += \
    module_mpic.o \
    module_mpif.o

load_DEFS = -DFEAT_MPI_LOADER=1
endif

load_BINS = libearld.so
load_DEPS = $(SRCDIR)/common/libcommon.a
load_PATH = $(DESTDIR)/lib
load_PERM = 0775

ifeq ($(FEAT_MPI_LOADER), 1)
load_BINS += libearld.python.so
endif

######## RULES

all: $(load_BINS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

module_mpi.o: module_mpi.c module_mpi.h
	$(CC) $(CFLAGS) $(load_DEFS) -c $<

libearld.so: $(load_OBJS) $(load_DEPS)
	$(CC) $(CC_FLAGS) -shared -o $@ $^ -ldl

libearld.python.so: $(load_OBJS) $(load_DEPS) 
	$(CC) $(CFLAGS) -DPMPI_DISABLED -c module_mpic.c
	$(CC) $(CFLAGS) -DPMPI_DISABLED -c module_mpif.c
	$(CC) $(CC_FLAGS) -shared -o $@ $^ -ldl

######## OPTIONS 

install: load.ginstall;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
