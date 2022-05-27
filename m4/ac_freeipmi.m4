##*****************************************************************************
## $Id$
##*****************************************************************************
#  AUTHOR:
#    Jordi Gómez
#
#  SYNOPSIS:
#    X_AC_FREEIPMI()
#
#  DESCRIPTION:
#    Check the usual suspects for a freeipmi installation,
#    updating CFLAGS and LDFLAGS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************


AC_DEFUN([X_AC_FREEIPMI_FIND_ROOT_DIR],
[
	for d in $_x_ac_freeipmi_dirs_root;
	do
		test -d "$d" || continue
		test -f "$d/include/freeipmi/freeipmi.h" || continue

		AS_VAR_SET(_cv_freeipmi_dir_root, $d)
		test -n "$_cv_freeipmi_dir_root" && break
	done
])

AC_DEFUN([X_AC_FREEIPMI],
[
	_x_ac_freeipmi_dirs_root="/usr /usr/local /opt/freeipmi"
	_x_ac_freeipmi_dirs_incl="include include64"

	AC_ARG_WITH(
		[freeipmi],
		AS_HELP_STRING(--with-freeipmi=PATH,Specify path to FREEIPMI installation),
		[
			_x_ac_freeipmi_dirs_root="$withval"
			_x_ac_freeipmi_custom="yes"
		]
	)

    AC_CACHE_CHECK(
        [for FREEIPMI root directory],
        [_cv_freeipmi_dir_root],
        [
            X_AC_FREEIPMI_FIND_ROOT_DIR([])
        
            if test -z "$_cv_freeipmi_dir_root"; then
                _x_ac_freeipmi_dirs_root="${_ax_ld_dirs_root}"
                _x_ac_freeipmi_custom="yes"
            
                X_AC_FREEIPMI_FIND_ROOT_DIR([])
            fi
        ]
    )

	if test -z "$_cv_freeipmi_dir_root"; then
		echo checking for FREEIPMI CFLAGS... no
	else
		FREEIPMI_DIR=$_cv_freeipmi_dir_root
		FREEIPMI_CFLAGS="-I\$(FREEIPMI_BASE)/include -DFREEIPMI_BASE=\\\"\$(FREEIPMI_BASE)\\\""
		FREEIPMI_LDFLAGS="-L\$(FREEIPMI_BASE)/lib -lfreeipmi -Wl,-rpath,\$(FREEIPMI_BASE)/lib -rdynamic"
		echo checking for FREEIPMI CFLAGS... $FREEIPMI_CFLAGS
		echo checking for FREEIPMI LDFLAGS... $FREEIPMI_LDFLAGS
	fi

	AC_SUBST(FREEIPMI_DIR)
	AC_SUBST(FREEIPMI_CFLAGS)
	AC_SUBST(FREEIPMI_LDFLAGS)

	AM_CONDITIONAL(WITH_FREEIPMI, test -n "$_cv_freeipmi_dir_root")
])
