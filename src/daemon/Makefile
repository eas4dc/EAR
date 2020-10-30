CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) $(CONSTANTS) 
LDFLAGS = $(DB_LDFLAGS) -rdynamic -lm -pthread -ldl -lm

######## INSTALLATION

eard_BINS = eard
eard_PATH = $(DESTDIR)/sbin
eard_PERM = 0770
oths_BINS = eard_api.o

######## FILES

eard_OBJS = \
    eard_checkpoint.o \
    eard_rapi.o \
    eard_server_api.o \
    dynamic_configuration.o \
		dyn_conf_theading.o \
    power_monitor.o \
	  app_mgt.o \
    shared_configuration.o \
    app_api/app_server_api.o \
    node_metrics.o

eard_OBJS += \
    $(SRCDIR)/database_cache/eardbd_api.o \
    $(SRCDIR)/metrics/libmetrics.a \
    $(SRCDIR)/common/libcommon.a \

######## RULES

all: all.components $(eard_BINS) $(oths_BINS) all.others

eard: eard.c $(eard_OBJS)
	$(CC) $(CFLAGS) $(DB_CFLAGS) -o $@ $< $(eard_OBJS) $(LDFLAGS) -lpthread

power_monitor.o: power_monitor.c power_monitor.h 
	$(CC) $(CFLAGS) -c $<


node_metrics.o:node_metrics.c node_metrics.h
	$(CC) $(CFLAGS) -c $<

dynamic_configuration.o: dynamic_configuration.c dynamic_configuration.h
	$(CC) $(CFLAGS) -c $<

dyn_conf_theading.o: dyn_conf_theading.c dyn_conf_theading.h
	$(CC) $(CFLAGS) -c $<

shared_configuration.o: shared_configuration.c shared_configuration.h
	$(CC) $(CFLAGS) -fPIC -c $<

eard_api.o: eard_api.c eard_api.h
	$(CC) $(CFLAGS) -fPIC -c $<

eard_rapi.o: eard_rapi.c eard_rapi.h
	$(CC) $(CFLAGS) -fPIC -c $<

eard_server_api.o: eard_server_api.c eard_server_api.h
	$(CC) $(CFLAGS) -c $< 

eard_checkpoint.o: eard_checkpoint.c eard_checkpoint.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: install.components install.others eard.ginstall;

%.install: %.install.components %.install.others;

clean: clean.components clean.others eard.gclean rclean;

######## SUBFOLDERS

%.components:
	$(MAKE) -C 'app_api' $*

%.others: ;

######## DEPENDENCIES

depend: depend.components depend.others

depend-clean: depend-clean.components depend-clean.others

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
