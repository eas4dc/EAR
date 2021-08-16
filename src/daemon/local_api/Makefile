CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) $(CONSTANTS) 

######## INSTALLATION


######## FILES

local_api_OBJS = \
	eard_api.o

######## RULES

all: $(local_api_OBJS) all.others
eard_api.o: eard_api.c eard_api.h
	$(CC) $(CFLAGS) -fPIC -c $<



######## OPTIONS


clean: local_api.gclean rclean;

######## SUBFOLDERS

%.others: ;

######## DEPENDENCIES

install:
%.install: ;

depend: depend.others

depend-clean:  depend-clean.others

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
