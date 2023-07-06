CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC $(CONSTANTS) $(DB_CFLAGS)

DB_MYSQL_LDFLAGS = $(DB_LDFLAGS) $(SRCDIR)/common/libcommon.a
DB_PSQL_LDFLAGS  = $(DB_LDFLAGS) -lpq $(SRCDIR)/common/libcommon.a


# Examon requirements
EXAMON_BASE = 
EXAMON_CFLAGS = -I$(EXAMON_BASE)/lib/iniparser/src -I$(EXAMON_BASE)/lib/mosquitto-1.3.5/lib
EXAMON_LDFLAGS = -L/usr/lib -L$(EXAMON_BASE)/lib/iniparser
LIBMOSQ = $(EXAMON_BASE)/lib/mosquitto-1.3.5/lib/libmosquitto.a
MOSQUITTO_LDFLAGS = -lm $(LIBMOSQ) -lssl -lcrypto -lrt -lpthread -liniparser


######## FILES

repo_PERM = 0775
repo_PATH = $(DESTDIR)/lib/plugins/reports
repo_DEPS = $(SRCDIR)/common/libcommon.a
repo_OBJS = report.o
repo_BINS = \
    eard.so \
    eardbd.so \
    csv.so \
    csv_ts.so \
    heroes.so \
    dummy.so \
		dcdb.so \
		sysfs.so \
    mpitrace.so \
	mpi_node_metrics.so
ifeq ($(FEAT_DB_PSQL),1)
repo_BINS += \
	psql.so 
endif
ifeq ($(FEAT_DB_MYSQL),1)
repo_BINS += \
	mysql.so 
endif
ifeq ($(FEAT_DB_CASSANDRA),1)
repo_BINS += \
	cassandra.so 
endif

ifeq ($(FEAT_EXAMON),1)
repo_BINS += \
        examon.so
endif

ifeq ($(FEAT_DB_PROMETHEUS), 1)
repo_BINS += \
		prometheus.so
endif

######## RULES

all: $(repo_OBJS) $(repo_BINS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

%.so: %.c
	$(CC) $(CFLAGS) -shared -o $@ $^

dcdb.so:dcdb.o
	$(CC) $(CFLAGS) -shared -o dcdb.so dcdb.o -lrt $(LDFLAGS)

eard.so: eard.c $(SRCDIR)/daemon/local_api/eard_api.c $(repo_DEPS)
	$(CC) $(CFLAGS) -shared -o $@ $^

eardbd.so: eardbd.c $(SRCDIR)/database_cache/eardbd_api.c $(repo_DEPS)
	$(CC) $(CFLAGS) -shared -o $@ $^

prometheus.so: prometheus.c $(repo_DEPS)
	$(CC) $(CFLAGS) -shared -o prometheus.so prometheus.c -lmicrohttpd 

mysql.so: mysql.c $(repo_DEPS)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(DB_LDFLAGS) $(SRCDIR)/common/libcommon.a

psql.so: psql.c $(repo_DEPS)
	$(CC) $(CFLAGS) $(DB_CFLAGS) -shared -o $@ $^ $(DB_LDFLAGS) $(SRCDIR)/common/libcommon.a

ifeq ($(FEAT_EXAMON),1)
examon.so: examon.c
	$(CC) $(CFLAGS) -shared -o examon.so examon.c $(EXAMON_CFLAGS) $(EXAMON_LDFLAGS) $(MOSQUITTO_LDFLAGS)
endif


cassandra.so: cassandra.c $(repo_DEPS)
	$(CC) $(CFLAGS) $(DB_CFLAGS) -shared -o $@ $^ -L/usr/lib64 -lcassandra

heroes.so: heroes.c $(repo_DEPS)
	$(CC) $(CFLAGS)  -shared -o $@ $^ -L/usr/lib64 

#$(CC) $(CFLAGS) $(DB_CFLAGS) -shared -o $@ $^ $(DB_LDFLAGS) -l:/home/xlalonso/downloads/cpp-driver/build/libcassandra.so
######## SUBFOLDERS

%.components: ;

######## OPTIONS

install: repo.ginstall

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
