CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) -fstack-protector-strong $(CONSTANTS)
LDFLAGS = -lm
ifeq ($(FEAT_RESEARCH), 1)
CFLAGS += -DEXT_OPT=1
endif
CCC     = $(CC) $(CFLAGS)

######## INSTALLATION

com1_PATH = $(DESTDIR)/bin
com1_PERM = 2771
com2_PATH = $(DESTDIR)/sbin
com2_PERM = 0770
com1_BINS = ear-info econtrol

ifdef DB_BASE
com1_BINS += \
    eacct \
    ereport

com2_BINS += \
    edb_create \
    edb_clean_apps \
    edb_clean_pm
endif

######## FILES

db_OBJS = \
			$(SRCDIR)/common/libdb.a 

comm_OBJS = \
			$(SRCDIR)/common/libcommon.a 

rapi_OBJS = \
    $(SRCDIR)/daemon/remote_api/eard_rapi_internals.o \
    $(SRCDIR)/daemon/remote_api/eard_rapi.o \
    $(SRCDIR)/common/messaging/msg_internals.o \
    $(SRCDIR)/global_manager/eargm_rapi.o \
    $(SRCDIR)/database_cache/eardbd_api.o

######## RULES

all: $(com1_BINS) $(com2_BINS)

eacct: ear_acct.c $(db_OBJS) $(comm_OBJS)
	$(CCC) $(SLURM_CFLAGS) $(DB_CFLAGS) -o $@ $< $(db_OBJS) $(comm_OBJS) $(DB_LDFLAGS) $(LDFLAGS)

econtrol: ear_control.c $(rapi_OBJS) $(comm_OBJS)
	$(CCC) $(SLURM_CFLAGS)  -o $@ $< $(rapi_OBJS) $(comm_OBJS) $(LDFLAGS)

ear-info: ear-info.c $(comm_OBJS)
	$(CCC) ${CFLAGS} -o $@ $< $(comm_OBJS) 

ereport: ear_report.c $(db_OBJS) $(comm_OBJS)
	$(CCC) $(DB_CFLAGS) -o $@ $< $(db_OBJS) $(comm_OBJS) $(DB_LDFLAGS) $(LDFLAGS)

edb_clean_pm: ear_clean_pm.c $(db_OBJS) $(comm_OBJS)
	$(CCC) $(DB_CFLAGS) -o $@ $< $(db_OBJS) $(comm_OBJS) $(DB_LDFLAGS) $(LDFLAGS)

edb_clean_apps: ear_clean_apps.c $(db_OBJS) $(comm_OBJS)
	$(CCC) $(DB_CFLAGS) -o $@ $< $(db_OBJS) $(comm_OBJS) $(DB_LDFLAGS) $(LDFLAGS)

edb_create: ear_create_database.c $(db_OBJS) $(comm_OBJS)
	$(CCC) $(DB_CFLAGS) -o $@ $< $(db_OBJS) $(comm_OBJS) $(DB_LDFLAGS) $(LDFLAGS)

######## OPTIONS

install: com1.ginstall com2.ginstall

%.install: ;

clean: com1.gclean com2.gclean rclean

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
