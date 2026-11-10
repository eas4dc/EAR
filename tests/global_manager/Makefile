# Tests for the EARGM powercap plugin manager (src/global_manager/plugins).
#
#   make check   needs a built tree (libcommon.a)

TOPDIR  = $(shell cd ../.. && pwd)
SRCDIR  = $(TOPDIR)/src
PCTEST  = $(TOPDIR)/tests/daemon/powercap

CC      ?= gcc
CFLAGS  = -g -O0 -Wall -I$(SRCDIR) -I$(PCTEST)
LDLIBS  = -lpthread -lm -ldl

all: test_powercap_plugin fake_egm_a.so fake_egm_b.so

fake_egm_a.so: fake_egm_plugin.c
	$(CC) $(CFLAGS) -fPIC -shared -DPLUG_ID=0 -o $@ $<
fake_egm_b.so: fake_egm_plugin.c
	$(CC) $(CFLAGS) -fPIC -shared -DPLUG_ID=1 -o $@ $<

test_powercap_plugin: test_powercap_plugin.c $(SRCDIR)/global_manager/plugins/powercap_plugin.c
	$(CC) $(CFLAGS) -rdynamic -o $@ $^ $(SRCDIR)/common/libcommon.a $(LDLIBS)

check: all
	./test_powercap_plugin

clean:
	rm -f test_powercap_plugin fake_egm_a.so fake_egm_b.so

.PHONY: all check clean
