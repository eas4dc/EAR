CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) -pthread $(DB_CFLAGS)
LDFLAGS = $(DB_LDFLAGS) -lpthread -lm -ldl

######## INSTALLATION

egmplug_PATH = $(DESTDIR)/lib/plugins/eargm
egmplug_PERM = 0775

######## FILES

egmplug_OBJS = \
    powercap_plugin.o 

egmplug_BINS = \
    dummy.so \
    fifo_plugin.so 

######## RULES

all: $(egmplug_OBJS) $(egmplug_BINS)

%.so: %.c
	$(CC) $(CFLAGS) -shared -o $@ $^ $(SRCDIR)/common/libcommon.a -fPIC

%.o: %.c %.h
	$(CC) $(CFLAGS) -fPIC -c $<

######## OPTIONS

install: egmplug.ginstall;

%.install: ;

clean: egmplug.gclean rclean;

######## DEPENDENCIES

depend: ;

depend-clean: ;

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
