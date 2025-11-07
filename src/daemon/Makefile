CFLAGS  = $(CC_FLAGS) -O2 -I$(SRCDIR) $(CONSTANTS) -Wall $(DB_CFLAGS)
LDFLAGS = -lm -pthread -ldl -rdynamic 

######## INSTALLATION

eard_BINS = eard
eard_PATH = $(DESTDIR)/sbin
eard_PERM = 0770

######## FILES

local_api_OBJS = \
	local_api/eard_api.o\
	local_api/eard_api_rpc.o \
	local_api/node_mgr.o

remote_api_OBJS= \
    remote_api/eard_server_api.o \
    remote_api/dyn_conf_theading.o \
    remote_api/dynamic_configuration.o 

report_OBJ=\
    $(SRCDIR)/report/report.o

powercap_OBJS = \
	powercap/powercap_status.o \
	powercap/powercap_mgt.o \
	powercap/powercap.o 

gpu_mgt_OBJS = \
	gpu/gpu_mgt.o

eard_OBJS = \
    eard_node_services.o\
    eard_checkpoint.o \
    power_monitor.o \
    app_mgt.o \
    shared_configuration.o \
    shared_pmon.o \
    app_server_api.o \
    node_metrics.o \
    log_eard.o \
	common.o

eard_OBJS += \
    $(remote_api_OBJS) \
    $(powercap_OBJS) \
    $(gpu_mgt_OBJS) \
    $(local_api_OBJS) \
    $(report_OBJ) \
    $(SRCDIR)/management/libmgt.a \
    $(SRCDIR)/metrics/libmetrics.a \
    $(SRCDIR)/common/libcommon.a

eard_LDFLAGS = $(call whole,$(SRCDIR)/metrics/common/libmetrics-common.a)

######## RULES

all: all.components $(eard_BINS) $(oths_BINS) all.others shared_pmon.o

eard: eard.c $(eard_OBJS)
	$(CC) $(CFLAGS) $(DB_CFLAGS) -o $@ $< $(eard_LDFLAGS) $(eard_OBJS) $(LDFLAGS) -lpthread

power_monitor.o: power_monitor.c power_monitor.h 
	$(CC) $(CFLAGS) -c $<  $(SLURM_CFLAGS)

node_metrics.o:node_metrics.c node_metrics.h
	$(CC) $(CFLAGS) -c $<

eard_node_services.o:eard_node_services.c local_api/eard_api.h local_api/eard_api_conf.h local_api/eard_api_rpc.h
	$(CC) $(CFLAGS) -c $<

shared_configuration.o: shared_configuration.c shared_configuration.h
	$(CC) $(CFLAGS) -fPIC -c $<

shared_pmon.o: shared_pmon.c shared_pmon.h power_monitor_app.h
	$(CC) $(CFLAGS) -fPIC -c $<

eard_checkpoint.o: eard_checkpoint.c eard_checkpoint.h
	$(CC) $(CFLAGS) -c $<

app_server_api.o: app_api/app_server_api.c app_api/app_server_api.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: install.components install.others eard.ginstall;

%.install: %.install.components %.install.others;

clean: clean.components clean.others rclean; #Do not add eard.gclean

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
