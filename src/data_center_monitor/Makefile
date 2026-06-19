CFLAGS = $(CC_FLAGS) -I$(SRCDIR)
LDFLAGS = -lm -lpthread -ldl

LDFLAGS += $(OPENSSL_LDFLAGS)

######## RULES

edcmon_BINS = edcmon
edcmon_PATH = $(DESTDIR)/sbin
edcmon_PERM = 0770


all: all.components $(edcmon_BINS)

edcmon: edcmon.c
	$(CC) $(CFLAGS) -rdynamic -o $@ $^ $(call whole,$(SRCDIR)/common/libcommon.a) $(LDFLAGS)

######## OPTIONS

install: install.components edcmon.ginstall;

%.install: %.install.components;

clean: clean.components rclean
	rm -f earpmd edcmon

######## SUBFOLDERS

%.components: ;
	@ $(MAKE) -C 'plugins' $* || exit

######## DEPENDENCIES

depend: depend.components

depend-clean: depend-clean.components

include $(SRCDIR)/Makefile.extra
