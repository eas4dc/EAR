######## FILES

OBJS = \
    ear_api.o \
    process_MPI.o \
    mpi_definitions_c.o \
    mpi_interception_c.o \
    MPI_interface.o

ifeq ($(FEAT_FORT), 1)
OBJS += \
    mpi_definitions_f.o \
    mpi_interception_f.o \
    MPI_interfaceF.o
endif

######## RULES

all: $(OBJS)

allt: DEFINES=-DEAR_GUI
allt: $(OBJS)

%.o: %.c %.h
	$(MPICC) $(MPICC_FLAGS) -Wall $(CC_FLAGS) $(CFLAGS) -c $<

######## OPTIONS 

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
