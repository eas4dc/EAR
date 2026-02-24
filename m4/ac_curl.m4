############################################################################
# Copyright (c) 2024 Energy Aware Solutions, S.L
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
############################################################################

AC_DEFUN([X_AC_CURL],
[
AC_ARG_WITH([curl],
  [AS_HELP_STRING([--with-curl=DIR], [Ruta de instalación de libcurl])],
  [CURL_DIR="$withval"],
  [CURL_DIR=""]
)

HAVE_CURL=0
MYCURL_CFLAGS=""
MYCURL_LIBS=""

# ----------------------------------------------------------------------
# Caso 1: El usuario pasa --with-curl=DIR explícitamente
# ----------------------------------------------------------------------
if test -n "$CURL_DIR"; then

  CURL_INC="$CURL_DIR/include/curl/curl.h"
  CURL_SO_DIR="$CURL_DIR/lib64"
  CURL_SO="$CURL_SO_DIR/libcurl.so"

  MYCURL_CFLAGS="-I$CURL_DIR/include"
  MYCURL_LIBS="-L$CURL_SO_DIR -lcurl"

  if test -f "$CURL_INC"; then
    AC_MSG_NOTICE([$CURL_INC found])
  else
    AC_MSG_ERROR([$CURL_INC not found])
  fi

  if ls "$CURL_SO_DIR/libcurl."* >/dev/null 2>&1; then
    AC_MSG_NOTICE([libcurl found at $CURL_SO_DIR])
  else
    AC_MSG_ERROR([libcurl not found at $CURL_SO_DIR])
  fi

  HAVE_CURL=1

# ----------------------------------------------------------------------
# Caso 2: No se indica ruta → intentar autodetección en el sistema
# ----------------------------------------------------------------------
else
  AC_MSG_NOTICE([--with-curl not specified; trying pkgconf...])

  PKG_CHECK_MODULES([CURL], [libcurl], [
      HAVE_CURL=1
      MYCURL_CFLAGS="$CURL_CFLAGS"
      MYCURL_LIBS="$CURL_LIBS"
      AC_MSG_NOTICE([libcurl found])
    ], [
      AC_MSG_WARN([libcurl not found])
      HAVE_CURL=0
    ]
  )
fi


# Exportar variables al Makefile
AC_SUBST([HAVE_CURL])
AC_SUBST([MYCURL_CFLAGS])
AC_SUBST([MYCURL_LIBS])

])
