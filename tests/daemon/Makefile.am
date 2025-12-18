## Process this file with automake to produce Makefile.in

## This variable is redefined because the main Makefile defines DESTDIR
## as as the prefix, but Autmake uses DESTDIR as a prefix of $(prefix) following
## the GNU standard and $(prefix) as the install target directory. This definition
## must be removed when all the project will use Automake.
DESTDIR=

AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -I$(top_srcdir)/src $(CONSTANTS)

check_PROGRAMS = local_api_test
local_api_test_SOURCES = $(top_srcdir)/src/daemon/local_api/eard_api.c \
						 $(top_srcdir)/src/daemon/local_api/eard_api_rpc.c
local_api_test_LDFLAGS = -Wl,--wrap=write -Wl,--wrap=read
local_api_test_LDADD = $(top_builddir)/src/common/libcommon.a $(CMOCKA_LDFLAGS) $(CMOCKA_LIBS) -lm
local_api_test_CPPFLAGS = -DTEST=1 $(CMOCKA_CPPFLAGS) $(AM_CPPFLAGS)
