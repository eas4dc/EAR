############################################################################
# Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
############################################################################
# 
# This program is part of the EAR software.
# 
# EAR provides a dynamic, transparent and ligth-weigth solution for
# Energy management. It has been developed in the context of the
# Barcelona Supercomputing Center (BSC)&Lenovo Collaboration project.
# 
# 
# BSC Contact   mailto:ear-support@bsc.es
# 
# 
# EAR is an open source software, and it is licensed under both the BSD-3 license
# and EPL-1.0 license. Full text of both licenses can be found in COPYING.BSD
# and COPYING.EPL files.
# 


# Function to append a string to an environment variable
AC_DEFUN([APPEND_TO_ENV_VAR],
[
	AC_SUBST([$1], ["${$1} $2"])
])

# Function to pop a string from an environment variable
AC_DEFUN([POP_FROM_ENV_VAR],
[
	temp_var="${$1}"
	temp_len=${#2}
	temp_var="${temp_var%$2}"
	AC_SUBST([$1], ["$temp_var"])
])


# Let configure script to accept the flag --with-dlb
AC_DEFUN([X_AC_DLB],
[
	AC_ARG_WITH([dlb],
		[AS_HELP_STRING([--with-dlb=<PATH>],
										[Specify a valid DLB root path to provide support for colleting MPI node metrics by using DLB TALP API (disabled by default).])],
		[],
		[with_dlb=no]
	)

	# AC_ARG_VAR([DLB_ROOT], [Specify the root path of DLB. If set, '--with-dlb' flag must also be set.])
	FEAT_DLB=0
	HAVE_DLB_TALP_H=0
	AS_IF([test "x$with_dlb" != xno],
				[

				 # Append -L/path/to/dlb/lib to LDFLAGS to test later
				 APPEND_TO_ENV_VAR([LDFLAGS], ["-L$with_dlb/lib"])
				 # AS_ECHO(["LDFLAGS after appending: $LDFLAGS"])

				 # check availability of -ldlb
				 AC_SEARCH_LIBS([DLB_MonitoringRegionRegister], [dlb],
												[
												 AC_SUBST([DLB_LIBS], [-ldlb])
												 AC_SUBST([DLB_LDFLAGS], ["-L$with_dlb/lib"])

												 # After the test, remove the appended value
												 POP_FROM_ENV_VAR([LDFLAGS], ["-L$with_dlb/lib"])
												 # AS_ECHO(["LDFLAGS after removing: $LDFLAGS"])

												 # Append -I/path/to/dlb/include to CPPFLAGS to test later
												 APPEND_TO_ENV_VAR([CPPFLAGS], ["-I$with_dlb/include"])
												 # AS_ECHO(["CPPFLAGS after appending: $CPPFLAGS"])

												 AC_CHECK_HEADERS([dlb_talp.h],
																					[
																					HAVE_DLB_TALP_H=1

																					AC_SUBST([DLB_CPPFLAGS], ["-I$with_dlb/include"])

																					FEAT_DLB=1
																					# AC_DEFINE([DLB_SUPPORT], [1], [Defined to 1 if EAR is compiled with DLB support.])
																					# After the test, remove the appended value
																					POP_FROM_ENV_VAR([CPPFLAGS], ["-I$with_dlb/include"])
																					# AS_ECHO(["CPPFLAGS after removing: $CPPFLAGS"])
																					]
												)
												 ])
				 ])

	AC_SUBST([FEAT_DLB])
	AC_SUBST([HAVE_DLB_TALP_H])
])
