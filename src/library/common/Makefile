########

OBJS = library_shared_data.o utils.o

ifeq ($(FEAT_MPI_API), 1)
OBJS += \
    global_comm.o
endif

######## RULES

all: $(OBJS)

%.o: %.c %.h
	$(CC) $(CC_FLAGS) $(LIB_CFLAGS) $(CFLAGS) -c $<

global_comm.o: global_comm.c global_comm.h 
	$(CC) $(CC_FLAGS) $(LIB_CFLAGS) $(CFLAGS) -c $<

######## OPTIONS 

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
