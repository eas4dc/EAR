API_CFLAGS = $(CFLAGS) $(MPI_CFLAGS) 

######## FILES

OBJS = \
    ear.o \
    ear_mpi.o \
    ear_seq.o \
    mpic.o \
    mpif.o \

BINS = libear_seq.so

######## RULES

all: $(OBJS) $(BINS)

ear.o: ear.c ear.h
	$(CC) $(API_CFLAGS)  $(PAPI_CFLAGS) -c $<	

%.o: %.c %.h
	$(CC) $(API_CFLAGS) -c $<

libear_seq.so: ear_seq.o
	$(CC) -shared -o libear_seq.so ear_seq.o -ldl

######## OPTIONS 

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
