CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) $(CONSTANTS) -Wall

######## INSTALLATION


######## FILES

msg_OBJS = \
	msg_internals.o 

######## RULES

all: $(msg_OBJS) all.others

msg_internals.o: msg_internals.c msg_internals.h 
	$(CC) $(CFLAGS) -fPIC -c $<


######## OPTIONS

clean: messaging.gclean rclean;
install: ;

######## SUBFOLDERS

%.others: ;
%.install: ;


######## DEPENDENCIES

depend: depend.others

depend-clean:  depend-clean.others

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
