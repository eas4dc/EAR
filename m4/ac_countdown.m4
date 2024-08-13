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


# Let configure script to accept the flag --with-countdown
AC_DEFUN([X_AC_COUNTDOWN],
[
	AC_ARG_WITH([countdown],
		[AS_HELP_STRING([--with-countdown=<PATH>],
										[Specify a valid Countdown root path to provide support for loading the Countdown library (disabled by default).])],
		[],
		[with_countdown=no]
	)

	LIBCOUNTDOWN_BASE=0
	AS_IF([test "x$with_countdown" != xno],
				[
				 AS_ECHO_N(["Checking for Countdown $with_countdown path... "])
				 AS_IF([test -d $with_countdown],
							 [
								AS_ECHO([yes])
								AS_ECHO_N(["Checking for $with_countdown/lib/libcntd.so... "])

								AS_IF(
											[test -e $with_countdown/lib/libcntd.so],
											[
												AS_ECHO([yes])
												LIBCOUNTDOWN_BASE="$with_countdown/lib"
											],
											[
												AS_ECHO([no])
											]
										 )
								],
								[
								 AS_ECHO([no])
								]
							)
				])
	AC_SUBST([LIBCOUNTDOWN_BASE])
])
