## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir)/src $(CONSTANTS)

check_PROGRAMS = hopper_tests

hopper_tests_SOURCES = hopper.c hopper.h $(top_srcdir)/src/metrics/common/apis.h
hopper_tests_LDADD = $(top_srcdir)/src/common/libcommon.a $(CMOCKA_LDFLAGS) $(CMOCKA_LIBS)

hopper_tests_CPPFLAGS = -DTEST=1 $(CMOCKA_CPPFLAGS) $(AM_CPPFLAGS)
