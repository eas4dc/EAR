CFLAGS = $(CC_FLAGS) $(MPI_CFLAGS) -I$(SRCDIR) -O2 -fPIC

OBJS = \
    global_comm.o\
		library_shared_data.o


######## RULES

all: $(OBJS)

%.o: %.c %.h
	$(MPICC) $(CC_FLAGS) $(CFLAGS) -c $<

library_shared_data.o:library_shared_data.c 
	$(MPICC) $(MPICC_FLAGS) $(CFLAGS) -c $< 	

######## OPTIONS 

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
