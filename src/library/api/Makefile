######## FILES

OBJS = \
    ear.o \
    ear_seq.o \
    clasify.o \
    mpi_support.o \
    cupti.o

ifeq ($(FEAT_MPI_API), 1)
OBJS += \
    ear_mpi.o \
    mpic.o \
    mpif.o
endif

BINS = libear_seq.so

######## RULES

all: $(OBJS) $(BINS)

%.o: %.c %.h
	$(CC) $(CC_FLAGS) $(LIB_CFLAGS) $(CFLAGS) -c $<

libear_seq.so: ear_seq.o
	$(CC) $(CC_FLAGS) -shared -o $@ $^ -ldl

######## OPTIONS 

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
