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
#    X_AC_RSMI()
#
#  DESCRIPTION:
#    Check the usual suspects for a rsmi installation,
#    updating CFLAGS and LDFLAGS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************


AC_DEFUN([X_AC_RSMI_FIND_ROOT_DIR],
[
	for d in $_x_ac_rsmi_dirs_root;
	do
		test -d "$d" || continue
		test -f "$d/include/rocm_smi/rocm_smi.h" || continue

		AS_VAR_SET(_cv_rsmi_dir_root, $d)
		test -n "$_cv_rsmi_dir_root" && break
	done
])

AC_DEFUN([X_AC_RSMI],
[
	_x_ac_rsmi_dirs_root="/usr /usr/local /opt/rsmi"
	_x_ac_rsmi_dirs_incl="include include64"

	AC_ARG_WITH(
		[rsmi],
		AS_HELP_STRING(--with-rsmi=PATH,Specify path to AMD ROCM-SMI installation),
		[
			_x_ac_rsmi_dirs_root="$withval"
			_x_ac_rsmi_custom="yes"
		]
	)

    AC_CACHE_CHECK(
        [for RSMI root directory],
        [_cv_rsmi_dir_root],
        [
            X_AC_RSMI_FIND_ROOT_DIR([])

            if test -z "$_cv_rsmi_dir_root"; then
                _x_ac_rsmi_dirs_root="${_ax_ld_dirs_root}"
                _x_ac_rsmi_custom="yes"

                X_AC_RSMI_FIND_ROOT_DIR([])
            fi
        ]
    )

	if test -z "$_cv_rsmi_dir_root"; then
		echo checking for RSMI CFLAGS... no
	else
		RSMI_DIR=$_cv_rsmi_dir_root
		RSMI_CFLAGS="-I\$(RSMI_BASE)/include -DRSMI_BASE=\\\"\$(RSMI_BASE)\\\""
		echo checking for RSMI CFLAGS... $RSMI_CFLAGS
	fi

	AC_SUBST(RSMI_DIR)
    AC_SUBST(RSMI_CFLAGS)

	AM_CONDITIONAL(WITH_RSMI, test -n "$_cv_rsmi_dir_root")
])
