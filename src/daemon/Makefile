CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) $(CONSTANTS) -Wall
LDFLAGS = $(DB_LDFLAGS) -lm -pthread -ldl -rdynamic 

######## INSTALLATION

eard_BINS = eard
eard_PATH = $(DESTDIR)/sbin
eard_PERM = 0770

######## FILES

local_api_OBJS = \
	local_api/eard_api.o

remote_api_OBJS= \
    remote_api/eard_server_api.o \
    remote_api/dyn_conf_theading.o \
    remote_api/dynamic_configuration.o 

powercap_OBJS = \
	powercap/powercap_status.o \
	powercap/powercap_mgt.o \
	powercap/powercap.o 

gpu_mgt_OBJS = \
	gpu/gpu_mgt.o

eard_OBJS = \
    eard_checkpoint.o \
    power_monitor.o \
	app_mgt.o \
    shared_configuration.o \
    app_api/app_server_api.o \
    node_metrics.o 

eard_OBJS += \
    $(remote_api_OBJS) \
    $(powercap_OBJS) \
    $(gpu_mgt_OBJS) \
    $(SRCDIR)/database_cache/eardbd_api.o \
    $(SRCDIR)/management/libmgt.a \
    $(SRCDIR)/metrics/libmetrics.a \
	$(local_api_OBJS) \
    $(SRCDIR)/common/libcommon.a

######## RULES

all: all.components $(eard_BINS) $(oths_BINS) all.others

eard: eard.c $(eard_OBJS)
	$(CC) $(CFLAGS) $(DB_CFLAGS) -o $@ $< $(eard_OBJS) $(LDFLAGS) -lpthread

power_monitor.o: power_monitor.c power_monitor.h 
	$(CC) $(CFLAGS) -c $<

node_metrics.o:node_metrics.c node_metrics.h
	$(CC) $(CFLAGS) -c $<

shared_configuration.o: shared_configuration.c shared_configuration.h
	$(CC) $(CFLAGS) -fPIC -c $<


eard_checkpoint.o: eard_checkpoint.c eard_checkpoint.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: install.components install.others eard.ginstall;

%.install: %.install.components %.install.others;

clean: clean.components clean.others eard.gclean rclean;

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'app_api' $*
	@ $(MAKE) -C 'remote_api' $*
	@ $(MAKE) -C 'local_api' $*
	@ $(MAKE) -C 'powercap' $*
	@ $(MAKE) -C 'gpu' $*

%.others: ;

######## DEPENDENCIES

depend: depend.components depend.others

depend-clean: depend-clean.components depend-clean.others

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
