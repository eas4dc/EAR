##*****************************************************************************
## $Id$
##*****************************************************************************
#  AUTHOR:
#    Jordi Gómez
#
#  SYNOPSIS:
#    X_AC_PAPI()
#
#  DESCRIPTION:
#    Check the usual suspects for a papi installation,
#    updating CPPFLAGS and LDFLAGS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************

AC_DEFUN([X_AC_PAPI_FIND_ROOT_DIR],
[
	for d in $_x_ac_papi_dirs_root; do
		test -d "$d" || continue
		test -d "$d/include" || continue
		test -f "$d/include/papi.h" || continue
		for dir_lib in $_x_ac_papi_dirs_libs;
		do
			# Testing if the library folder exists
			test -d $d/$dir_lib || continue

			# If exists, then its path and LDFLAGS are saved
			if test -d "$d/$dir_lib"; then
				_x_ac_papi_dir_lib="$d/$dir_lib"

				if test "x$_x_ac_papi_custom" = "xyes"; then
					_x_ac_papi_gcc_ldflags=-L$_x_ac_papi_dir_lib
				fi
			fi

			X_AC_VAR_BACKUP([],[$_x_ac_papi_gcc_ldflags],[$_x_ac_papi_gcc_libs])
			#
			# AC_LANG_CALL(prologue, function) creates a source file
			# and calls to the function.
			# AC_LINK_IFELSE(input, action-if-true) runs the compiler
			# and linker using LDFLAGS and LIBS.
			#
			AC_LINK_IFELSE(
				[AC_LANG_CALL([], PAPI_get_real_usec)],
				[PAPI_get_real_usec=yes]
			)
			X_AC_VAR_UNBACKUP

			if test "x$PAPI_get_real_usec" = "xyes"; then
				AS_VAR_SET(_cv_papi_dir_root, $d)
			fi

			test -n "$_cv_papi_dir_root" && break
		done
		test -n "$_cv_papi_dir_root" && break
	done
])

AC_DEFUN([X_AC_PAPI],
[
	_x_ac_papi_dirs_root="/usr /usr/local /opt/papi"
    _x_ac_papi_dirs_libs="lib64 lib"
    _x_ac_papi_gcc_libs="-lpapi"
    _x_ac_papi_gcc_ldflags=
    _x_ac_papi_dir_bin=
    _x_ac_papi_dir_lib=

    AC_ARG_WITH(
        [papi],
        AS_HELP_STRING(--with-papi=PATH,Specify path to PAPI installation),
        [
			_x_ac_papi_dirs_root="$withval"
			_x_ac_papi_custom="yes"
		]
    )

    AC_CACHE_CHECK(
        [for PAPI root directory],
        [_cv_papi_dir_root],
        [
			X_AC_PAPI_FIND_ROOT_DIR([])
			
    		if test -z "$_cv_papi_dir_root"; then
				_x_ac_papi_dirs_root="${_ax_ld_dirs_root}"
				_x_ac_papi_custom="yes"
				
				X_AC_PAPI_FIND_ROOT_DIR([])
			fi
        ]
    )

    if test -z "$_cv_papi_dir_root"; then
        echo checking for PAPI compiler link... no
    else
		PAPI_DIR=$_cv_papi_dir_root
        PAPI_LIBS=$_x_ac_papi_gcc_libs
		
		if test "x$_x_ac_papi_custom" = "xyes"; then
        	PAPI_LIBDIR=$_x_ac_papi_dir_lib
        	PAPI_CPPFLAGS="-I$PAPI_DIR/include"
        	PAPI_LDFLAGS="$_x_ac_papi_gcc_ldflags"

			if test "x$enable_rpath" != "xno"; then
        		PAPI_LDFLAGS="$PAPI_LDFLAGS -Wl,-rpath,$_x_ac_papi_dir_lib"
			fi
		fi
        
		echo checking for PAPI compiler link... yes
        echo checking for PAPI CPPFLAGS... $PAPI_CPPFLAGS
        echo checking for PAPI LDFLAGS... $PAPI_LDFLAGS
        echo checking for PAPI libraries... $PAPI_LIBS
    fi

    AC_SUBST(PAPI_LIBS)
    AC_SUBST(PAPI_LIBDIR)
    AC_SUBST(PAPI_CPPFLAGS)
    AC_SUBST(PAPI_LDFLAGS)
    AC_SUBST(PAPI_DIR)

    AM_CONDITIONAL(WITH_PAPI, test -n "$_cv_papi_dir_root")
])
