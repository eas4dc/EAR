##*****************************************************************************
## $Id$
##*****************************************************************************
#  AUTHOR:
#    Jordi Gómez
#
#  SYNOPSIS:
#    X_AC_REDFISH()
#
#  DESCRIPTION:
#    Check the usual suspects for a redfish installation,
#    updating CPPFLAGS and LDFLAGS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************

AC_DEFUN([X_AC_REDFISH_FIND_ROOT_DIR],
[
	for d in $_x_ac_redfish_dirs_root; do
		test -d "$d" || continue
		test -d "$d/include" || continue
		test -f "$d/include/redfish.h" || continue
		for dir_lib in $_x_ac_redfish_dirs_libs;
		do
			# Testing if the library folder exists
			test -d $d/$dir_lib || continue

			# If exists, then its path and LDFLAGS are saved
			if test -d "$d/$dir_lib"; then
				_x_ac_redfish_dir_lib="$d/$dir_lib"

				if test "x$_x_ac_redfish_custom" = "xyes"; then
					_x_ac_redfish_gcc_ldflags=-L$_x_ac_redfish_dir_lib
				fi
			fi

			# Also its bin folder is saved
			if test -d "$d/bin"; then
				_x_ac_redfish_dir_bin="$d/bin"
			fi

			X_AC_VAR_BACKUP([],[$_x_ac_redfish_gcc_ldflags],[$_x_ac_redfish_gcc_libs])
			#
			# AC_LANG_CALL(prologue, function) creates a source file
			# and calls to the function.
			# AC_LINK_IFELSE(input, action-if-true) runs the compiler
			# and linker using LDFLAGS and LIBS.
			#
			AC_LINK_IFELSE(
				[AC_LANG_CALL([], createServiceEnumerator)],
				[createServiceEnumerator=yes]
			)
			X_AC_VAR_UNBACKUP

			if test "x$createServiceEnumerator" = "xyes"; then
				AS_VAR_SET(_cv_redfish_dir_root, $d)
			fi

			test -n "$_cv_redfish_dir_root" && break
		done
		test -n "$_cv_redfish_dir_root" && break
	done
])

AC_DEFUN([X_AC_REDFISH],
[
    _x_ac_redfish_dirs_root="/usr /usr/local /opt/redfish"
    _x_ac_redfish_dirs_libs="lib64 lib"
    _x_ac_redfish_gcc_libs="-lredfish"
    _x_ac_redfish_gcc_ldflags=
    _x_ac_redfish_dir_bin=
    _x_ac_redfish_dir_lib=

    AC_ARG_WITH(
        [redfish],
        AS_HELP_STRING(--with-redfish=PATH,Specify path to REDFISH installation),
        [
			_x_ac_redfish_dirs_root="$withval"
			_x_ac_redfish_custom="yes"
		]
    )

    AC_CACHE_CHECK(
        [for REDFISH root directory],
        [_cv_redfish_dir_root],
        [
			X_AC_REDFISH_FIND_ROOT_DIR([])

    		if test -z "$_cv_redfish_dir_root"; then
				_x_ac_redfish_dirs_root="${_ax_ld_dirs_root}"
				_x_ac_redfish_custom="yes"

				X_AC_REDFISH_FIND_ROOT_DIR([])
			fi
        ]
    )

    if test -z "$_cv_redfish_dir_root"; then
        echo checking for REDFISH compiler link... no
    else
        REDFISH_DIR=$_cv_redfish_dir_root
        REDFISH_LIBS=$_x_ac_redfish_gcc_libs
        
		if test "x$_x_ac_redfish_custom" = "xyes"; then
        	REDFISH_LIBDIR=$_x_ac_redfish_dir_lib
        	REDFISH_CFLAGS='-I$(REDFISH_BASE)/include -DREDFISH_BASE=\"$(REDFISH_BASE)\"'
		fi

        echo checking for REDFISH compiler link... yes
        echo checking for REDFISH CFLAGS... $REDFISH_CFLAGS
        echo checking for REDFISH libraries... $REDFISH_LIBS
    fi

    AC_SUBST(REDFISH_DIR)
    AC_SUBST(REDFISH_CFLAGS)

    AM_CONDITIONAL(WITH_REDFISH, test -n "$_cv_redfish_dir_root")
])
