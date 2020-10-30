CFLAGS  = $(CC_FLAGS) $(MPI_CFLAGS) -I$(SRCDIR) -O2 -fPIC
LDFLAGS = $(SRCDIR)/common/libcommon.a -ldl

######## FILES

load_OBJS = \
    loader.o \
    module_mpi.o \
    module_mpic.o \
    module_mpif.o \
    module_default.o

load_BINS = \
    libearld.so

load_DEPS = $(SRCDIR)/common/libcommon.a

load_PATH = $(DESTDIR)/lib
load_PERM = 0775

######## RULES

all: clean $(load_OBJS) $(load_BINS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

libearld.so:
	$(CC) -shared -o libearld.so $(load_OBJS) $(LDFLAGS) $(load_DEPS)

######## OPTIONS 

install: load.ginstall;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
