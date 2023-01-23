CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) -pthread $(DB_FLAGS)
LDFLAGS = $(DB_LDFLAGS) -lpthread -lm -ldl

######## INSTALLATION

eargmd_PATH = $(DESTDIR)/sbin
eargmd_PERM = 0770

######## FILES

comm_BINS = \
    $(SRCDIR)/daemon/remote_api/eard_rapi.o \
    $(SRCDIR)/daemon/remote_api/eard_rapi_internals.o \
    $(SRCDIR)/common/libcommon.a

eargmd_OBJS = \
    eargm_rapi.o

ifdef DB_BASE
eargmd_OBJS += \
    cluster_powercap.o \
    cluster_energycap.o \
    meta_eargm.o \
    eargm_ext_rm.o

eargmd_BINS = \
    eargmd
endif

######## RULES

all: $(eargmd_OBJS) $(eargmd_BINS)

eargmd: eargm.c $(comm_BINS) $(eargmd_OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(eargmd_OBJS) $(comm_BINS) $(LDFLAGS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -fPIC -c $<

######## OPTIONS

install: eargmd.ginstall;

%.install: ;

clean: eargmd.gclean rclean;

######## DEPENDENCIES

depend: ;

depend-clean: ;

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
