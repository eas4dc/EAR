CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) -pthread
LDFLAGS = $(DB_LDFLAGS) -lpthread -lm

######## INSTALLATION

eargmd_BINS = eargmd
eargmd_PATH = $(DESTDIR)/sbin
eargmd_PERM = 0770

######## FILES

comm_BINS = $(SRCDIR)/daemon/remote_api/eard_rapi.o \
    $(SRCDIR)/common/libcommon.a 

eargmd_OBJS = eargm_rapi.o \
		cluster_powercap.o \
		cluster_energycap.o \
		eargm_ext_rm.o 

######## RULES

all: $(eargmd_OBJS) $(eargmd_BINS) all.others

eargmd: eargm.c $(comm_BINS) $(eargmd_OBJS)
	$(CC) $(CFLAGS) $(DB_CFLAGS) -o $@ $< $(eargmd_OBJS) $(comm_BINS) $(LDFLAGS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -fPIC -c $<

######## OPTIONS

install: install.others eargmd.ginstall;

%.install: %.install.others;

clean: clean.others eargmd.gclean rclean;

######## SUBFOLDERS

%.others: ;

######## DEPENDENCIES

depend: depend.others

depend-clean: depend-clean.others

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
