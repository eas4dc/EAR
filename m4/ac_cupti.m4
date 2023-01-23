##*****************************************************************************
## $Id$
##*****************************************************************************
#  AUTHOR:
#    Jordi Gómez
#
#  SYNOPSIS:
#    X_AC_CUPTI()
#
#  DESCRIPTION:
#    Check the usual suspects for a cupti installation,
#    updating CFLAGS and LDFLAGS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************

AC_DEFUN([X_AC_CUPTI_FIND_ROOT_DIR],
[
	for d in $_x_ac_cupti_dirs_root;
	do
		test -d "$d" || continue
		test -f "$d/include/cupti.h" || continue

		AS_VAR_SET(_cv_cupti_dir_root, $d)
		test -n "$_cv_cupti_dir_root" && break
	done
])

AC_DEFUN([X_AC_CUPTI],
[
	_x_ac_cupti_dirs_root="/usr /usr/local /opt/cupti"
	_x_ac_cupti_dirs_incl="include include64"

	AC_ARG_WITH(
		[cupti],
		AS_HELP_STRING(--with-cupti=PATH,Specify path to CUPTI installation),
		[
			_x_ac_cupti_dirs_root="$withval"
			_x_ac_cupti_custom="yes"
		]
	)

    AC_CACHE_CHECK(
        [for CUPTI root directory],
        [_cv_cupti_dir_root],
        [
            X_AC_CUPTI_FIND_ROOT_DIR([])

            if test -z "$_cv_cupti_dir_root"; then
                _x_ac_cupti_dirs_root="${_ax_ld_dirs_root}"
                _x_ac_cupti_custom="yes"

                X_AC_CUPTI_FIND_ROOT_DIR([])
            fi
        ]
        )


	if test -z "$_cv_cupti_dir_root"; then
		echo checking for CUPTI CFLAGS... no
	else
		CUPTI_DIR=$_cv_cupti_dir_root
		CUPTI_CFLAGS="\$(CUDA_CFLAGS) -I\$(CUPTI_BASE)/include -DCUPTI_BASE=\\\"\$(CUPTI_BASE)\\\""
		echo checking for CUPTI CFLAGS... $CUPTI_CFLAGS
	fi

	AC_SUBST(CUPTI_DIR)
    AC_SUBST(CUPTI_CFLAGS)

	AM_CONDITIONAL(WITH_CUPTI, test -n "$_cv_cupti_dir_root")
])
