CFLAGS = $(CC_FLAGS) $(MPI_CFLAGS) -I$(SRCDIR) -O2 -fPIC

######## FILES

OBJS = states.o \
       states_periodic.o

######## RULES

all: $(OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS 

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
