CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) -pthread $(DB_CFLAGS)
LDFLAGS = $(DB_LDFLAGS) -lpthread -lm -ldl

######## INSTALLATION

eargmd_PATH = $(DESTDIR)/sbin
eargmd_PERM = 0770

######## FILES

comm_BINS = \
    $(SRCDIR)/daemon/remote_api/eard_rapi.o \
    $(SRCDIR)/daemon/remote_api/eard_rapi_internals.o \
    $(SRCDIR)/common/libdb.a \
	$(SRCDIR)/report/report.o \
    $(SRCDIR)/common/libcommon.a 


eargmd_OBJS = \
    eargm_rapi.o


eargmd_DEPS = \
    plugins/powercap_plugin.o

eargmd_OBJS += \
    cluster_powercap.o \
    cluster_energycap.o \
    meta_eargm.o \
    eargm_ext_rm.o

egmplug_BINS = \
    dummy_plugin.so #\
    #default_plugin.so 

ifdef DB_BASE
ifeq ($(FEAT_WF_SUPPORT), 1)
eargmd_BINS = \
    eargmd
endif
endif

LDFLAGS += $(OPENSSL_LDFLAGS)

######## RULES

all: all.components $(eargmd_OBJS) $(eargmd_BINS)

eargmd: eargm.c $(comm_BINS) $(eargmd_DEPS) $(eargmd_OBJS)
	$(CC) $(CFLAGS) -o $@ $< $(eargmd_OBJS) $(eargmd_DEPS) $(comm_BINS) $(LDFLAGS)

%.so: %.c
	$(CC) $(CFLAGS) -shared -o $@ $^ $(SRCDIR)/common/libcommon.a -fPIC

%.o: %.c %.h
	$(CC) $(CFLAGS) -fPIC -c $<

######## OPTIONS

install: install.components eargmd.ginstall;

%.install: ;

clean: clean.components eargmd.gclean rclean;

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'plugins' $*

######## DEPENDENCIES

depend: ;

depend-clean: ;

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
