##*****************************************************************************
## $Id$
##*****************************************************************************
#  AUTHOR:
#    Jordi Gómez
#
#  SYNOPSIS:
#    X_AC_PBS()
#
#  DESCRIPTION:
#    Check the usual suspects for a pbs installation,
#    updating CPPFLAGS and LDFLAGS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************

AC_DEFUN([X_AC_PBS],
[
    AC_ARG_WITH(
        [pbs],
        AS_HELP_STRING(--with-pbs,Specify to enable PBS components),
        [
			_cv_pbs_dir_root=/
		]
    )
    
    if test -z "$_cv_pbs_dir_root"; then
        echo checking for PBS compiler link... no
    else
        echo checking for PBS compiler link... yes
	
        SCHED_DIR=$_cv_pbs_dir_root
        SCHED_NAME=PBS
    fi
])
