AC_DEFUN([X_AC_JANSSON],
[
AC_ARG_WITH([jansson],
  [AS_HELP_STRING([--with-jansson=DIR], [Ruta de instalación de libjansson])],
  [JANSSON_DIR="$withval"],
  [JANSSON_DIR=""]
)

HAVE_JANSSON=0
MYJANSSON_CFLAGS=""
MYJANSSON_LIBS=""

# ----------------------------------------------------------------------
# Caso 1: El usuario pasa --with-jansson=DIR explícitamente
# ----------------------------------------------------------------------
if test -n "$JANSSON_DIR"; then

  JANSSON_INC="$JANSSON_DIR/include/jansson.h"
  JANSSON_SO_DIR="$JANSSON_DIR/lib64"
  JANSSON_SO_ALT="$JANSSON_DIR/lib"

  # Preparamos rutas
  MYJANSSON_CFLAGS="-I$JANSSON_DIR/include"
  MYJANSSON_LIBS="-L$JANSSON_SO_DIR -ljansson"

  # Comprobar headers
  if test -f "$JANSSON_INC"; then
    AC_MSG_NOTICE([Encontrado jansson.h en $JANSSON_DIR/include])
  else
    AC_MSG_ERROR([No se encontró jansson.h en $JANSSON_DIR/include])
  fi

  # Comprobar librería en lib64
  if ls "$JANSSON_SO_DIR/libjansson."* >/dev/null 2>&1; then
    AC_MSG_NOTICE([Encontrada libjansson en $JANSSON_SO_DIR])
  else
    # fallback a lib
    if ls "$JANSSON_SO_ALT/libjansson."* >/dev/null 2>&1; then
      AC_MSG_NOTICE([Encontrada libjansson en $JANSSON_SO_ALT])
      MYJANSSON_LIBS="-L$JANSSON_SO_ALT -ljansson"
    else
      AC_MSG_ERROR([No se encontró libjansson en $JANSSON_SO_DIR ni en $JANSSON_SO_ALT])
    fi
  fi

  HAVE_JANSSON=1

# ----------------------------------------------------------------------
# Caso 2: Autodetección del sistema con pkg-config
# ----------------------------------------------------------------------
else
  AC_MSG_NOTICE([No se especificó --with-jansson; intentando autodetección con pkg-config])

  PKG_CHECK_MODULES([JANSSON], [jansson], [
      HAVE_JANSSON=1
      MYJANSSON_CFLAGS="$JANSSON_CFLAGS"
      MYJANSSON_LIBS="$JANSSON_LIBS"
      AC_MSG_NOTICE([jansson detectado mediante pkg-config])
    ], [
      AC_MSG_WARN([No se encontró jansson en el sistema])
      HAVE_JANSSON=0
    ]
  )
fi

# ----------------------------------------------------------------------
# Exportar variables
# ----------------------------------------------------------------------
AC_SUBST([HAVE_JANSSON])
AC_SUBST([MYJANSSON_CFLAGS])
AC_SUBST([MYJANSSON_LIBS])

])

