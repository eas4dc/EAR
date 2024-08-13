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
#    Julita Corbalan
#
#  SYNOPSIS:
#    X_AC_OAR()
#
#  DESCRIPTION:
#    Check the usual suspects for a pbs installation,
#    updating CPPFLAGS and LDFLAGS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************

AC_DEFUN([X_AC_OAR],
[
    AC_ARG_WITH(
        [oar],
        AS_HELP_STRING(--with-oar,Specify to enable OAR components),
        [
			_cv_oar_dir_root=/
		]
    )
    
    if test -z "$_cv_oar_dir_root"; then
        echo checking for OAR compiler link... no
    else
        echo checking for OAR compiler link... yes
	
        SCHED_DIR=$_cv_oar_dir_root
        SCHED_NAME=OAR
    fi
])
