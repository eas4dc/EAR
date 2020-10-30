##*****************************************************************************
## $Id$
##*****************************************************************************
#  AUTHOR:
#    Jordi Gómez
#
#  SYNOPSIS:
#    X_AC_GSL()
#
#  DESCRIPTION:
#    Check the usual suspects for a gsl installation,
#    updating CPPFLAGS and LDFLAGS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************

AC_DEFUN([X_AC_GSL_FIND_ROOT_DIR],
[
	for d in $_x_ac_gsl_dirs_root; do
		test -d "$d" || continue
		test -d "$d/include" || continue
		test -d "$d/include/gsl" || continue
		test -f "$d/include/gsl/gsl_cblas.h" || continue
		for dir_lib in $_x_ac_gsl_dirs_libs;
		do
			# Testing if the library folder exists
			test -d $d/$dir_lib || continue

			# If exists, then its path and LDFLAGS are saved
			if test -d "$d/$dir_lib"; then
				_x_ac_gsl_dir_lib="$d/$dir_lib"

				if test "x$_x_ac_gsl_custom" = "xyes"; then
					_x_ac_gsl_gcc_ldflags=-L$_x_ac_gsl_dir_lib
				fi
			fi

			# Also its bin folder is saved
			if test -d "$d/bin"; then
				_x_ac_gsl_dir_bin="$d/bin"
			fi

			X_AC_VAR_BACKUP([],[$_x_ac_gsl_gcc_ldflags],[$_x_ac_gsl_gcc_libs])
			#
			# AC_LANG_CALL(prologue, function) creates a source file
			# and calls to the function.
			# AC_LINK_IFELSE(input, action-if-true) runs the compiler
			# and linker using LDFLAGS and LIBS.
			#
			AC_LINK_IFELSE(
				[AC_LANG_CALL([], cos)],
				[cos=yes]
			)
			AC_LINK_IFELSE(
				[AC_LANG_CALL([], cblas_dgemm)],
				[cblas_dgemm=yes]
			)
			AC_LINK_IFELSE(
				[AC_LANG_CALL([], gsl_blas_dgemm)],
				[gsl_blas_dgemm=yes]
			)
			X_AC_VAR_UNBACKUP

			if test "x$cos" = "xyes" \
			&& test "x$cblas_dgemm" = "xyes" \
			&& test "x$gsl_blas_dgemm" = "xyes"; then
				AS_VAR_SET(_cv_gsl_dir_root, $d)
			fi

			test -n "$_cv_gsl_dir_root" && break
		done
		test -n "$_cv_gsl_dir_root" && break
	done
])

AC_DEFUN([X_AC_GSL],
[
    _x_ac_gsl_dirs_root="/usr /usr/local /opt/gsl"
    _x_ac_gsl_dirs_libs="lib64 lib"
    _x_ac_gsl_gcc_libs="-lm -lgsl -lgslcblas"
    _x_ac_gsl_gcc_ldflags=
    _x_ac_gsl_dir_bin=
    _x_ac_gsl_dir_lib=

    AC_ARG_WITH(
        [gsl],
        AS_HELP_STRING(--with-gsl=PATH,Specify path to GSL installation),
        [
			_x_ac_gsl_dirs_root="$withval"
			_x_ac_gsl_custom="yes"
		]
    )

    AC_CACHE_CHECK(
        [for GSL root directory],
        [_cv_gsl_dir_root],
        [
			X_AC_GSL_FIND_ROOT_DIR([])

    		if test -z "$_cv_gsl_dir_root"; then
				_x_ac_gsl_dirs_root="${_ax_ld_dirs_root}"
				_x_ac_gsl_custom="yes"

				X_AC_GSL_FIND_ROOT_DIR([])
			fi
        ]
    )

    if test -z "$_cv_gsl_dir_root"; then
        echo checking for GSL compiler link... no
    else
        GSL_DIR=$_cv_gsl_dir_root
        GSL_LIBS=$_x_ac_gsl_gcc_libs
        
		if test "x$_x_ac_gsl_custom" = "xyes"; then
        	GSL_LIBDIR=$_x_ac_gsl_dir_lib
        	GSL_CPPFLAGS="-I$GSL_DIR/include"
        	GSL_LDFLAGS=$_x_ac_gsl_gcc_ldflags
		fi

        echo checking for GSL compiler link... yes
        echo checking for GSL CPPFLAGS... $GSL_CPPFLAGS
        echo checking for GSL LDFLAGS... $GSL_LDFLAGS
        echo checking for GSL libraries... $GSL_LIBS
    fi

    AC_SUBST(GSL_LIBS)
    AC_SUBST(GSL_LIBDIR)
    AC_SUBST(GSL_CPPFLAGS)
    AC_SUBST(GSL_LDFLAGS)
    AC_SUBST(GSL_DIR)

    AM_CONDITIONAL(WITH_GSL, test -n "$_cv_gsl_dir_root")
])
