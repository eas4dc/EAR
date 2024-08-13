############################################################################
# Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
############################################################################
AC_DEFUN([AX_SET_ARCHITECTURES],
[
	AC_ARG_ENABLE([arm64],
		AS_HELP_STRING([--enable-arm64], [Compiles for ARM64 architecture]))

    if test "x$enable_arm64" = "xyes"; then
        ARCH=ARM
    else
        ARCH=X86
    fi

    AC_SUBST(ARCH)
])
