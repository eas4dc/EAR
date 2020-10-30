##*****************************************************************************
## $Id$
##*****************************************************************************
#  AUTHOR:
#    Jordi Gómez
#
#  SYNOPSIS:
#    X_AC_SLURM()
#
#  DESCRIPTION:
#    Check the usual suspects for a slurm installation,
#    updating CPPFLAGS and LDFLAGS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************

AC_DEFUN([X_AC_SLURM_FIND_ROOT_DIR],
[
	for d in $_x_ac_slurm_dirs_root; do
		test -d "$d" || continue
		test -d "$d/include" || continue
		test -d "$d/include/slurm" || continue
		test -f "$d/include/slurm/slurm.h" || continue
		test -f "$d/include/slurm/spank.h" || continue
		for dir_lib in $_x_ac_slurm_dirs_libs;
		do
			# Testing if the library folder exists
			test -d $d/$dir_lib || continue

			# If exists, then its path and LDFLAGS are saved
			if test -d "$d/$dir_lib"; then
				_x_ac_slurm_dir_lib="$d/$dir_lib"

				if test "x$_x_ac_slurm_custom" = "xyes"; then
					_x_ac_slurm_gcc_ldflags=-L$_x_ac_slurm_dir_lib
				fi
			fi

			X_AC_VAR_BACKUP([],[$_x_ac_slurm_gcc_ldflags],[$_x_ac_slurm_gcc_libs])
			#
			# AC_LANG_CALL(prologue, function) creates a source file
			# and calls to the function.
			# AC_LINK_IFELSE(input, action-if-true) runs the compiler
			# and linker using LDFLAGS and LIBS.
			#
			AC_LINK_IFELSE(
				[AC_LANG_CALL([], slurm_ping)],
				[slurm_ping=yes]
			)
			X_AC_VAR_UNBACKUP

			if test "x$slurm_ping" = "xyes"; then
				AS_VAR_SET(_cv_slurm_dir_root, $d)
			fi

			test -n "$_cv_slurm_dir_root" && break
		done
		test -n "$_cv_slurm_dir_root" && break
	done
])

AC_DEFUN([X_AC_SLURM],
[
    _x_ac_slurm_dirs_root="/usr /usr/local /opt/slurm"
    _x_ac_slurm_dirs_libs="lib64 lib"
    _x_ac_slurm_gcc_libs="-lslurm"
    _x_ac_slurm_gcc_ldflags=
    _x_ac_slurm_dir_bin=
    _x_ac_slurm_dir_lib=

    AC_ARG_WITH(
        [slurm],
        AS_HELP_STRING(--with-slurm=PATH,Specify path to SLURM installation),
        [
			_x_ac_slurm_dirs_root="$withval"
			_x_ac_slurm_custom="yes"
		]
    )

    AC_CACHE_CHECK(
        [for SLURM root directory],
        [_cv_slurm_dir_root],
        [
			X_AC_SLURM_FIND_ROOT_DIR([])

				if test -z "$_cv_slurm_dir_root"; then
					_x_ac_slurm_dirs_root="${_ax_ld_dirs_root}"
					_x_ac_slurm_custom="yes"

					X_AC_SLURM_FIND_ROOT_DIR([])
				fi
        ]
    )

    if test -z "$_cv_slurm_dir_root"; then
        echo checking for SLURM compiler link... no
    else
        SLURM_DIR=$_cv_slurm_dir_root
        SLURM_LIBS=$_x_ac_slurm_gcc_libs
		
		if test "x$_x_ac_slurm_custom" = "xyes"; then
        	SLURM_LIBDIR=$_x_ac_slurm_dir_lib
       		SLURM_CPPFLAGS="-I$SLURM_DIR/include"
        	SLURM_LDFLAGS=$_x_ac_slurm_gcc_ldflags
		fi

        echo checking for SLURM compiler link... yes
        echo checking for SLURM CPPFLAGS... $SLURM_CPPFLAGS
        echo checking for SLURM LDFLAGS... $SLURM_LDFLAGS
        echo checking for SLURM libraries... $SLURM_LIBS
    fi

    AC_SUBST(SLURM_LIBS)
    AC_SUBST(SLURM_LIBDIR)
    AC_SUBST(SLURM_CPPFLAGS)
    AC_SUBST(SLURM_LDFLAGS)
    AC_SUBST(SLURM_DIR)

    AM_CONDITIONAL(WITH_SLURM, test -n "$_cv_slurm_dir_root")
])
