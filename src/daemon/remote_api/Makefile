CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) $(CONSTANTS)  -Wall

######## INSTALLATION


######## FILES

remote_api_OBJS = \
    eard_rapi.o \
    eard_server_api.o \
    dynamic_configuration.o \
	dyn_conf_theading.o 

######## RULES

all: $(remote_api_OBJS) all.others

dynamic_configuration.o: dynamic_configuration.c dynamic_configuration.h
	$(CC) $(CFLAGS) -c $<

dyn_conf_theading.o: dyn_conf_theading.c dyn_conf_theading.h
	$(CC) $(CFLAGS) -c $<

eard_rapi_internals.o: eard_rapi_internals.c eard_rapi_internals.h 
	$(CC) $(CFLAGS) -fPIC -c $<

eard_rapi.o: eard_rapi.c eard_rapi.h 
	$(CC) $(CFLAGS) -fPIC -c $<

eard_server_api.o: eard_server_api.c eard_server_api.h
	$(CC) $(CFLAGS) -c $< 


######## OPTIONS


clean: remote_api.gclean rclean;

######## SUBFOLDERS

%.others: ;

######## DEPENDENCIES
install:
%.install: ;

depend: depend.others

depend-clean:  depend-clean.others

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
