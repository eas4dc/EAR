##*****************************************************************************
## $Id$
##*****************************************************************************
#  AUTHOR:
#    Jordi Gómez
#
#  SYNOPSIS:
#    X_AC_ONEAPI()
#
#  DESCRIPTION:
#    Check the usual suspects for a OneAPI Level Zero installation,
#    updating CFLAGS and LDFLAGS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************


AC_DEFUN([X_AC_ONEAPI_FIND_ROOT_DIR],
[
	for d in $_x_ac_oneapi_dirs_root;
	do
		test -d "$d" || continue
		test -f "$d/include/level_zero/ze_api.h" || continue
		test -f "$d/include/level_zero/zes_ddi.h" || continue

		AS_VAR_SET(_cv_oneapi_dir_root, $d)
		test -n "$_cv_oneapi_dir_root" && break
	done
])

AC_DEFUN([X_AC_ONEAPI],
[
	_x_ac_oneapi_dirs_root="/usr /usr/local /opt/oneapi"
	_x_ac_oneapi_dirs_incl="include include64"

	AC_ARG_WITH(
		[oneapi],
		AS_HELP_STRING(--with-oneapi=PATH,Specify path to OneAPI Level Zero installation),
		[
			_x_ac_oneapi_dirs_root="$withval"
			_x_ac_oneapi_custom="yes"
		]
	)

    AC_CACHE_CHECK(
        [for ONEAPI root directory],
        [_cv_oneapi_dir_root],
        [
            X_AC_ONEAPI_FIND_ROOT_DIR([])

            if test -z "$_cv_oneapi_dir_root"; then
                _x_ac_oneapi_dirs_root="${_ax_ld_dirs_root}"
                _x_ac_oneapi_custom="yes"

                X_AC_ONEAPI_FIND_ROOT_DIR([])
            fi
        ]
    )

	if test -z "$_cv_oneapi_dir_root"; then
		echo checking for ONEAPI CFLAGS... no
	else
		ONEAPI_DIR=$_cv_oneapi_dir_root
		ONEAPI_CFLAGS="-I\$(ONEAPI_BASE)/include -DONEAPI_BASE=\\\"\$(ONEAPI_BASE)\\\""
		echo checking for OneAPI Level Zero CFLAGS... $ONEAPI_CFLAGS
	fi

	AC_SUBST(ONEAPI_CFLAGS)
	AC_SUBST(ONEAPI_DIR)

	AM_CONDITIONAL(WITH_ONEAPI, test -n "$_cv_oneapi_dir_root")
])
