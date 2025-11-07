## Process this file with automake to produce Makefile.in

## This variable is redefined because the main Makefile defines DESTDIR
## as as the prefix, but Autmake uses DESTDIR as a prefix of $(prefix) following
## the GNU standard and $(prefix) as the install target directory. This definition
## must be removed when all the project will use Automake.
DESTDIR=

AUTOMAKE_OPTIONS = subdir-objects

lib_LIBRARIES = libear_api.a

include_HEADERS = ear.h

AM_CPPFLAGS = -I$(top_srcdir)/src $(CONSTANTS)

libear_api_a_SOURCES = app_api.c app_conf_api.h \
											 $(top_srcdir)/src/common/system/poll.c $(top_srcdir)/src/common/system/poll.h \
											 $(top_srcdir)/src/common/environment_common.c $(top_srcdir)/src/common/environment_common.h

etc.install: ;

# libear_api_a_LIBDADD = $(top_srcdir)/src/common/libcommon.la
