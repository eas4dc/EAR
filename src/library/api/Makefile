######## FILES

OBJS = \
    ear.o \
    ear_seq.o \
    clasify.o \
    mpi_support.o \
    cupti.o \
    eard_dummy.o

ifeq ($(FEAT_MPI_LIBRARY), 1)
OBJS += \
    ear_mpi.o \
    mpic.o \
    mpif.o
endif

BINS = libear_seq.so
oths_BINS = earld.h
oths_PATH = $(DESTDIR)/include
oths_PERM = 0664

dummyld_BINS = libearld_dummy.a
dummyld_PATH = $(DESTDIR)/lib
dummyld_PERM = 0664

######## RULES

all: $(OBJS) $(BINS) $(dummyld_BINS)

%.o: %.c %.h
	$(CC) $(CC_FLAGS) $(LIB_CFLAGS) $(CFLAGS) -c $<

libear_seq.so: ear_seq.o
	$(CC) $(CC_FLAGS) -shared -o $@ $^ -ldl

libearld_dummy.a:earld.o
	ar rvs libearld_dummy.a earld.o

######## OPTIONS 

install: oths.ginstall dummyld.ginstall;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
