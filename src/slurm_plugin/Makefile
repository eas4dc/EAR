CFLAGS  = -fPIC -O2 $(SLURM_CFLAGS) -I$(SRCDIR)
LDFLAGS = -lm

######## INSTALLATION

erun_BINS = erun
erun_PATH = $(DESTDIR)/bin
erun_PERM = 0775

ifdef SLURM_BASE
plug_BINS = earplug.so
endif
plug_PATH = $(DESTDIR)/lib
plug_PERM = 0775

######## FILES

erun_SRCS = \
        erun.c \
        erun_sim.c \
        erun_lock.c

plug_OBJS = \
        $(SRCDIR)/daemon/remote_api/eard_rapi.o \
        $(SRCDIR)/daemon/shared_configuration.o \
        $(SRCDIR)/global_manager/eargm_rapi.o \
        $(SRCDIR)/common/libcommon.a 

plug_SRCS = \
        slurm_plugin.c \
        slurm_plugin_environment.c \
        slurm_plugin_options.c \
        slurm_plugin_rcom_eard.c \
        slurm_plugin_rcom_eargmd.c \
        slurm_plugin_rcom_shared.c \
        slurm_plugin_serialization.c

plug_HDRS = \
        slurm_plugin.h \
        slurm_plugin_environment.h \
        slurm_plugin_options.h \
        slurm_plugin_rcom.h \
        slurm_plugin_serialization.h

######## RULES

all: $(erun_BINS) $(plug_BINS)

earplug.so: $(plug_SRCS) $(plug_HDRS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared -o $@ $(plug_SRCS) $(plug_OBJS) $(LDFLAGS)

erun: $(erun_SRCS) $(plug_SRCS) $(plug_HDRS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -DERUN -o $@ $(erun_SRCS) $(plug_SRCS) $(plug_OBJS) $(LDFLAGS)

######## OPTIONS

install: erun.ginstall plug.ginstall;

%.install: ;

clean: erun.gclean plug.gclean rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
