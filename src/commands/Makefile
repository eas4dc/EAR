CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) -Wall 
LDFLAGS = -lm
ifeq ($(FEAT_RESEARCH), 1)
CFLAGS += -DEXT_OPT=1
endif
CCC     = $(CC) $(CFLAGS)


######## INSTALLATION

ifdef DB_BASE
com1_BINS = eacct econtrol ereport ear-info
com1_PATH = $(DESTDIR)/bin
com1_PERM = 2771
com2_BINS = edb_create edb_clean_apps edb_clean_pm
com2_PATH = $(DESTDIR)/sbin
com2_PERM = 0770
endif

######## FILES

comm_OBJS = $(SRCDIR)/common/libcommon.a -lm

rapi_OBJS = $(SRCDIR)/daemon/remote_api/eard_rapi.o $(SRCDIR)/common/messaging/msg_internals.o $(SRCDIR)/database_cache/eardbd_api.o

######## RULES

all: $(com1_BINS) $(com2_BINS)

eacct: ear_acct.c $(comm_OBJS)
	$(CCC) $(SLURM_CFLAGS) $(DB_CFLAGS) -o $@ $< $(comm_OBJS) $(DB_LDFLAGS)

econtrol: ear_control.c $(rapi_OBJS) $(comm_OBJS)
	$(CCC) $(SLURM_CFLAGS)  -o $@ $< $(rapi_OBJS) $(comm_OBJS)

ear-info: ear-info.c $(comm_OBJS)
	$(CCC) ${CFLAGS} -o $@ $< $(comm_OBJS)

ereport: ear_report.c $(comm_OBJS)
	$(CCC) $(DB_CFLAGS) -o $@ $< $(comm_OBJS) $(DB_LDFLAGS)

edb_clean_pm: ear_clean_pm.c $(comm_OBJS)
	$(CCC) $(DB_CFLAGS) -o $@ $< $(comm_OBJS) $(DB_LDFLAGS)

edb_clean_apps: ear_clean_apps.c $(comm_OBJS)
	$(CCC) $(DB_CFLAGS) -o $@ $< $(comm_OBJS) $(DB_LDFLAGS)

edb_create: ear_create_database.c $(comm_OBJS)
	$(CCC) $(DB_CFLAGS) -o $@ $< $(comm_OBJS) $(DB_LDFLAGS)

######## OPTIONS

install: com1.ginstall com2.ginstall

%.install: ;

clean: com1.gclean com2.gclean rclean

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
