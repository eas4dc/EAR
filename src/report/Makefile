CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC $(CONSTANTS) $(DB_CFLAGS)

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
    mpitrace.so
ifeq ($(FEAT_DB_PSQL),1)
repo_BINS += \
	psql.so 
endif
ifeq ($(FEAT_DB_MYSQL),1)
repo_BINS += \
	mysql.so 
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

mysql.so: mysql.c $(repo_DEPS)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(DB_LDFLAGS)

psql.so: psql.c $(repo_DEPS)
	$(CC) $(CFLAGS) -shared -o $@ $^ $(DB_LDFLAGS)

######## SUBFOLDERS

%.components: ;

######## OPTIONS

install: repo.ginstall

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
