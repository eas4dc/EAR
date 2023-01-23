######## FILES

OBJS = \
	states_comm.o \
    states.o 

######## RULES

all: $(OBJS)

%.o: %.c %.h
	# DYNAIS_CUTOFF enables the dynamic DynAIS turning off.
	$(CC) $(CC_FLAGS) $(LIB_CFLAGS) $(CFLAGS) -DDYNAIS_CUTOFF=1 -c $<

######## OPTIONS 

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
