CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) $(GSL_CFLAGS) $(DB_CFLAGS)
LDFLAGS = $(GSL_LDFLAGS) -lm
CCC     = $(CC) $(CFLAGS)

######## INSTALLATION

tool_BINS = \
    coeffs_show \
    coeffs_default \
    coeffs_null \
    cluster_ping \
    ear_conf \
    insert_test \
    enode_info \
    node_manager_info \
    list_folders \
    groups_test \
	powercap_test

#enode_control

ifeq ($(FEAT_WF_SUPPORT), 1)
ifdef DB_BASE
tool_BINS += \
    learning_show \
    get_periodic \
    metrics_show \
    learning_delete
endif
ifdef GSL_BASE
tool_BINS += \
	coeffs_compute
endif
endif
tool_PATH = $(DESTDIR)/bin/tools
tool_PERM = 0770

######## FILES

rapi_OBJS = \
    $(SRCDIR)/daemon/remote_api/eard_rapi_internals.o \
    $(SRCDIR)/daemon/remote_api/eard_rapi.o \
    $(SRCDIR)/common/messaging/msg_internals.o

db_OJBS = \
    $(SRCDIR)/common/libdb.a

comm_OBJS = \
    $(SRCDIR)/common/libcommon.a

mang_OBJS = \
    $(SRCDIR)/management/libmgt.a \
	$(SRCDIR)/metrics/libmetrics.a 

######## RULES

all: $(tool_BINS)

powercap_test: powercap_test.c $(mang_OBJS) $(comm_OBJS)
	$(CCC) -rdynamic -o $@ $< $(LDFLAGS) $(mang_OBJS) $(SRCDIR)/daemon/local_api/libeard.a $(comm_OBJS) -lpthread -ldl -lm

coeffs_compute: coeffs_compute.c $(mang_OBJS) $(db_OJBS) $(comm_OBJS)
	$(CCC) -o $@ $< $(LDFLAGS) $(mang_OBJS) $(db_OJBS) $(comm_OBJS) $(DB_LDFLAGS) -lpthread -ldl

coeffs_show: coeffs_show.c $(comm_OBJS)
	$(CCC) -I$(SRCDIR) -o $@ $< $(comm_OBJS)

coeffs_null: coeffs_null.c $(comm_OBJS)
	$(CCC) -I$(SRCDIR) -o $@ $< $(comm_OBJS)

coeffs_default: coeffs_default.c $(comm_OBJS)
	$(CCC) -I$(SRCDIR) -o $@ $< $(comm_OBJS)

learning_show: learning_show.c $(db_OJBS) $(comm_OBJS)
	$(CCC) -o $@ $< $(db_OJBS) $(comm_OBJS) $(DB_LDFLAGS) -lm

metrics_show: metrics_show.c $(db_OJBS) $(comm_OBJS)
	$(CCC) -o $@ $< $(db_OJBS) $(comm_OBJS) $(DB_LDFLAGS) -lm

insert_test: insert_test.c $(comm_OBJS) $(SRCDIR)/report/report.o
	$(CCC) -o $@ $< $(SRCDIR)/report/report.o $(comm_OBJS) -ldl -lpthread -lm

get_periodic: get_periodic.c $(db_OJBS) $(comm_OBJS)
	$(CCC) -o $@ $< $(db_OJBS) $(comm_OBJS) $(DB_LDFLAGS) -lm

learning_delete: learning_delete.c $(db_OJBS) $(comm_OBJS)
	$(CCC) -o $@ $< $(db_OJBS) $(comm_OBJS) $(DB_LDFLAGS)

ear_conf: ear_conf.c $(comm_OBJS)
	$(CCC) -o $@ $< $(comm_OBJS)

enode_control: ear_node_control.c $(comm_OBJS)
	$(CCC) -o $@ $< $(comm_OBJS)

groups_test: groups_test.c $(comm_OBJS)
	$(CCC) -o $@ $< $(comm_OBJS)

enode_info: ear_node_info.c $(comm_OBJS) $(SRCDIR)/daemon/local_api/libeard.a $(SRCDIR)/metrics/libmetrics.a $(SRCDIR)/management/libmgt.a
	$(CCC) -o $@ $< $(SRCDIR)/daemon/local_api/libeard.a  $(SRCDIR)/management/libmgt.a $(SRCDIR)/metrics/libmetrics.a $(comm_OBJS) -pthread -lm -ldl

cluster_ping: cluster_ping.c $(comm_OBJS) $(rapi_OBJS)
	$(CCC) -o $@ $< $(rapi_OBJS) $(comm_OBJS) -lm

list_folders: list_folders.c $(comm_OBJS)
	$(CCC) -I$(SRCDIR) -o $@ $< $(comm_OBJS)

node_manager_info: node_manager_info.c $(comm_OBJS)
	$(CCC) -o $@ $<  $(SRCDIR)/daemon/local_api/libeard.a $(SRCDIR)/daemon/shared_configuration.o $(SRCDIR)/daemon/shared_pmon.o $(comm_OBJS)
######## OPTIONS

install: tool.ginstall;

%.install: ;

clean: tool.gclean rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
