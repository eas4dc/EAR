## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = subdir-objects

CONSTANTS = -DSEC_KEY=10001 -DESTIMATE_PERF=0 -DSYSTEM_TYPE=$(SYSTEM_TYPE) -DUSE_PVC_HWMON=$(USE_PVC_HWMON) $(OPENSSL_CONSTANT)
AM_CPPFLAGS = -I$(top_srcdir)/src $(CONSTANTS)
PYTHON = python3

check_PROGRAMS = test_status test_types reporting_client

common_library = $(top_builddir)/src/common/libcommon.a
common_sources = $(top_srcdir)/src/database_cache/eardbd_api.h \
								 $(top_srcdir)/src/database_cache/eardbd_api.c

test_status_SOURCES = test_status.c $(common_sources)
test_status_LDADD = $(common_library) -lm
test_status_CFLAGS = -Wno-unused-result
test_types_SOURCES = test_types.c $(common_sources)
test_types_LDADD = $(common_library) -lm
test_types_CFLAGS = -Wno-unused-result

reporting_client_SOURCES = reporting_client.c $(common_sources)
reporting_client_LDADD = $(common_library) -lm

check_LTLIBRARIES = record_a.la record_b.la

common_record_SRCS = reporting_plugin.c \
	$(top_srcdir)/src/report/report.h

record_a_la_SOURCES = $(common_record_SRCS)
record_a_la_CPPFLAGS = -DREPORT_ID='"A"' $(AM_CPPFLAGS)
# A check-only library without -rpath becomes a static convenience archive.
# This link directory enables loadable modules; it does not install them.
record_a_la_LDFLAGS = -module -avoid-version -rpath $(libdir)/plugins/reports

record_b_la_SOURCES = $(common_record_SRCS)
record_b_la_CPPFLAGS = -DREPORT_ID='"B"' $(AM_CPPFLAGS)
record_b_la_LDFLAGS = -module -avoid-version -rpath $(libdir)/plugins/reports

# make post reaches check: compile everything, but leave execution explicit.

.PHONY: check-reporting
check-reporting: check
	$(PYTHON) $(srcdir)/reporting_tests.py --client "$(abs_builddir)/reporting_client" $(REPORTING_ARGS)

EXTRA_DIST = reporting_plugin.c reporting_tests.py README.md
