AC_DEFUN([X_AC_MICROHTTPD],
[
AC_ARG_WITH([microhttpd],
  [AS_HELP_STRING([--with-microhttpd=DIR], [Ruta de instalación de libmicrohttpd])],
  [MICROHTTPD_DIR="$withval"],
  [MICROHTTPD_DIR=""]
)

HAVE_MICROHTTPD=0
MYMICROHTTPD_CFLAGS=""
MYMICROHTTPD_LIBS=""

# ----------------------------------------------------------------------
# Caso 1: Usuario pasa --with-microhttpd=DIR
# ----------------------------------------------------------------------
if test -n "$MICROHTTPD_DIR"; then

  MICROHTTPD_INC="$MICROHTTPD_DIR/include/microhttpd.h"
  MICROHTTPD_SO_DIR="$MICROHTTPD_DIR/lib64"
  MICROHTTPD_SO_ALT="$MICROHTTPD_DIR/lib"

  MYMICROHTTPD_CFLAGS="-I$MICROHTTPD_DIR/include"
  MYMICROHTTPD_LIBS="-L$MICROHTTPD_SO_DIR -lmicrohttpd"

  # Comprobar header
  if test -f "$MICROHTTPD_INC"; then
    AC_MSG_NOTICE([Encontrado microhttpd.h en $MICROHTTPD_DIR/include])
  else
    AC_MSG_ERROR([No se encontró microhttpd.h en $MICROHTTPD_DIR/include])
  fi

  # Comprobar librería en lib64
  if ls "$MICROHTTPD_SO_DIR/libmicrohttpd."* >/dev/null 2>&1; then
    AC_MSG_NOTICE([Encontrada libmicrohttpd en $MICROHTTPD_SO_DIR])
  else
    # fallback a lib
    if ls "$MICROHTTPD_SO_ALT/libmicrohttpd."* >/dev/null 2>&1; then
      AC_MSG_NOTICE([Encontrada libmicrohttpd en $MICROHTTPD_SO_ALT])
      MYMICROHTTPD_LIBS="-L$MICROHTTPD_SO_ALT -lmicrohttpd"
    else
      AC_MSG_ERROR([No se encontró libmicrohttpd en $MICROHTTPD_SO_DIR ni en $MICROHTTPD_SO_ALT])
    fi
  fi

  HAVE_MICROHTTPD=1

# ----------------------------------------------------------------------
# Caso 2: Autodetección en el sistema con pkg-config
# ----------------------------------------------------------------------
else
  AC_MSG_NOTICE([No se especificó --with-microhttpd; intentando autodetección con pkg-config])

  PKG_CHECK_MODULES([MICROHTTPD], [libmicrohttpd], [
      HAVE_MICROHTTPD=1
      MYMICROHTTPD_CFLAGS="$MICROHTTPD_CFLAGS"
      MYMICROHTTPD_LIBS="$MICROHTTPD_LIBS"
      AC_MSG_NOTICE([libmicrohttpd detectado mediante pkg-config])
    ], [
      AC_MSG_WARN([No se encontró libmicrohttpd en el sistema])
      HAVE_MICROHTTPD=0
    ]
  )
fi

# ----------------------------------------------------------------------
# Exportar variables para Makefile
# ----------------------------------------------------------------------
AC_SUBST([HAVE_MICROHTTPD])
AC_SUBST([MYMICROHTTPD_CFLAGS])
AC_SUBST([MYMICROHTTPD_LIBS])

])

