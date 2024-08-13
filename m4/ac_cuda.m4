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
#    X_AC_CUDA()
#
#  DESCRIPTION:
#    Check the usual suspects for a cuda installation,
#    updating CFLAGS and LDFLAGS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************


AC_DEFUN([X_AC_CUDA_FIND_ROOT_DIR],
[
	for d in $_x_ac_cuda_dirs_root;
	do
		test -d "$d" || continue
		test -f "$d/include/cuda.h" || continue
		test -f "$d/include/nvml.h" || continue

		AS_VAR_SET(_cv_cuda_dir_root, $d)
		test -n "$_cv_cuda_dir_root" && break
	done
])

AC_DEFUN([X_AC_CUDA],
[
	_x_ac_cuda_dirs_root="/usr /usr/local /opt/cuda"
	_x_ac_cuda_dirs_incl="include include64"

	AC_ARG_WITH(
		[cuda],
		AS_HELP_STRING(--with-cuda=PATH,Specify path to CUDA installation),
		[
			_x_ac_cuda_dirs_root="$withval"
			_x_ac_cuda_custom="yes"
		]
	)

    AC_CACHE_CHECK(
        [for CUDA root directory],
        [_cv_cuda_dir_root],
        [
            X_AC_CUDA_FIND_ROOT_DIR([])

            if test -z "$_cv_cuda_dir_root"; then
                _x_ac_cuda_dirs_root="${_ax_ld_dirs_root}"
                _x_ac_cuda_custom="yes"

                X_AC_CUDA_FIND_ROOT_DIR([])
            fi
        ]
    )

	if test -z "$_cv_cuda_dir_root"; then
		echo checking for CUDA CFLAGS... no
	else
		CUDA_DIR=$_cv_cuda_dir_root
		CUDA_CFLAGS="-I\$(CUDA_BASE)/include -DCUDA_BASE=\\\"\$(CUDA_BASE)\\\""
		echo checking for CUDA CFLAGS... $CUDA_CFLAGS
	fi

	AC_SUBST(CUDA_DIR)
	AC_SUBST(CUDA_CFLAGS)

	AM_CONDITIONAL(WITH_CUDA, test -n "$_cv_cuda_dir_root")
])
