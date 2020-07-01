CFLAGS  = $(CC_FLAGS) -I$(SRCDIR)
LDFLAGS = $(DB_LDFLAGS) -lpthread

######## INSTALLATION

eargmd_BINS = eargmd
eargmd_PATH = $(DESTDIR)/sbin
eargmd_PERM = 0770

######## FILES

comm_BINS = $(SRCDIR)/daemon/eard_rapi.o \
    $(SRCDIR)/common/libcommon.a \
    eargm_server_api.o \
		eargm_ext_rm.o

eargmd_OBJS = eargm_rapi.o \
    eargm_server_api.o

######## RULES

all: $(eargmd_OBJS) $(eargmd_BINS) all.others

eargmd: eargm.c $(comm_BINS)
	$(CC) $(CFLAGS) $(DB_CFLAGS) -o $@ $< $(comm_BINS) $(LDFLAGS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -fPIC -c $<

######## OPTIONS

install: install.others eargmd.ginstall;

%.install: %.install.others;

clean: clean.others eard.gclean rclean;

######## SUBFOLDERS

%.others: ;

######## DEPENDENCIES

depend: depend.others

depend-clean: depend-clean.others

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
