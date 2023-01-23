CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) -fPIC $(CONSTANTS)

######## INSTALLATION

######## FILES

lapi_OBJS = \
	eard_api.o \
	eard_api_rpc.o \
	node_mgr.o

######## RULES

all: libeard.a

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

libeard.a: $(lapi_OBJS)
	$(AR) rvs $@ $(lapi_OBJS)

######## OPTIONS

clean: lapi.gclean rclean;

######## SUBFOLDERS

%.others: ;

######## DEPENDENCIES

install:

%.install: ;

depend: depend.others

depend-clean:  depend-clean.others

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra

