##*****************************************************************************
## $Id$
##*****************************************************************************
#  AUTHOR:
#    Jordi Gómez
#
#  SYNOPSIS:
#    X_AC_CUDA()
#
#  DESCRIPTION:
#    Check the usual suspects for a likwid installation,
#    updating CFLAGS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************


AC_DEFUN([X_AC_LIKWID_FIND_ROOT_DIR],
[
	for d in $_x_ac_likwid_dirs_root;
	do
		test -d "$d" || continue
		test -f "$d/include/likwid.h" || continue

		AS_VAR_SET(_cv_likwid_dir_root, $d)
		test -n "$_cv_likwid_dir_root" && break
	done
])

AC_DEFUN([X_AC_LIKWID],
[
	_x_ac_likwid_dirs_root="/usr /usr/local /opt/likwid"
	_x_ac_likwid_dirs_incl="include include64"

	AC_ARG_WITH(
		[likwid],
		AS_HELP_STRING(--with-likwid=PATH,Specify path to LIKWID installation),
		[
			_x_ac_likwid_dirs_root="$withval"
			_x_ac_likwid_custom="yes"
		]
	)

    AC_CACHE_CHECK(
        [for LIKWID root directory],
        [_cv_likwid_dir_root],
        [
            X_AC_LIKWID_FIND_ROOT_DIR([])

            if test -z "$_cv_likwid_dir_root"; then
                _x_ac_likwid_dirs_root="${_ax_ld_dirs_root}"
                _x_ac_likwid_custom="yes"

                X_AC_LIKWID_FIND_ROOT_DIR([])
            fi
        ]
    )

	if test -z "$_cv_likwid_dir_root"; then
		echo checking for LIKWID CFLAGS... no
	else
		LIKWID_DIR=$_cv_likwid_dir_root
		LIKWID_CFLAGS="-I\$(LIKWID_BASE)/include -DLIKWID_BASE=\\\"\$(LIKWID_BASE)\\\""
		echo checking for LIKWID CFLAGS... $LIKWID_CFLAGS
	fi

	AC_SUBST(LIKWID_DIR)
	AC_SUBST(LIKWID_CFLAGS)

	AM_CONDITIONAL(WITH_LIKWID, test -n "$_cv_likwid_dir_root")
])
