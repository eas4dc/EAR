######## FILES

OBJS = states.o \
       states_periodic.o

######## RULES

all: $(OBJS)

allt: DEFINES=-DEAR_GUI
allt: $(OBJS) 

%.o: %.c %.h
	$(MPICC) $(CC_FLAGS) $(CFLAGS) $(DEFINES) -c $<

######## OPTIONS 

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
