############################################################################
# Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
############################################################################
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
#    updating CFLAGS and LDFLAGS as necessary.
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

		AS_VAR_SET(_cv_slurm_dir_root, $d)
        break
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
        AS_HELP_STRING(--with-slurm=PATH,Specify path to SLURM installation. Use no to disable),
        [
		    if test "x$withval" = "xno"; then
                _x_ac_slurm_force="no"
    		else
			    _x_ac_slurm_dirs_root="$withval"
                _x_ac_slurm_aux="$withval"
	    		_x_ac_slurm_custom="yes"
            fi
		]
    )
   
    # Forced no through --without-slurm 
    if test "x$_x_ac_slurm_force" != "xno"; then
    AC_CACHE_CHECK(
        [for SLURM root directory],
        [_cv_slurm_dir_root],
        [
			X_AC_SLURM_FIND_ROOT_DIR([])

                # If not found the SLURM dir in dirs provided by 
                # top definition or by --with-slurm, then use
                # the LD_LIBRARY_PATH dirs
				if test -z "$_cv_slurm_dir_root"; then
					_x_ac_slurm_dirs_root="${_ax_ld_dirs_root}"
					_x_ac_slurm_custom="yes"

					X_AC_SLURM_FIND_ROOT_DIR([])
				fi
        ]
    )
    else
        if test "x$SCHED_NAME" = "xSLURM"; then
            SCHED_NAME=""
        fi
    fi

    # Force custom (if custom = yes but dir (not dirs) root is empty)
    if test "x$_x_ac_slurm_custom" = "xyes" && test -z "$_cv_slurm_dir_root"; then
        _cv_slurm_dir_root="$_x_ac_slurm_aux"
	fi

    if test -z "$_cv_slurm_dir_root"; then
        echo checking for SLURM compiler link... no
    else
        SLURM_DIR=$_cv_slurm_dir_root
		if test "x$_x_ac_slurm_custom" = "xyes"; then
       		SLURM_CFLAGS="-I$SLURM_DIR/include"
		fi

        echo checking for SLURM compiler link... yes
        echo checking for SLURM CFLAGS... $SLURM_CFLAGS
	
        SCHED_DIR=$SLURM_DIR	
        SCHED_NAME=SLURM
    fi

    AC_SUBST(SLURM_CFLAGS)
    AC_SUBST(SLURM_DIR)

    AM_CONDITIONAL(WITH_SLURM, test -n "$_cv_slurm_dir_root")
])
