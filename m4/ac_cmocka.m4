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


AC_DEFUN([X_AC_CMOCKA],
		 [
			AC_ARG_WITH([cmocka],
						[AS_HELP_STRING([--with-cmocka=<PATH>],
										[Specify a valid cmocka root path to provide support to compile some tests.])
						],
						[],
						[with_cmocka=no]
						)
			WITH_CMOCKA=0
			AS_IF([test "x$with_cmocka" != xno],
				  [
					AX_VAR_PUSHVALUE([LDFLAGS], ["-L$with_cmocka/lib"])
					AX_VAR_PUSHVALUE([LIBS])

					# check availability of -lcmocka
					AC_SEARCH_LIBS([cmocka_set_message_output], [cmocka],
								   [
									AC_SUBST([CMOCKA_LIBS], [-lcmocka])
									AC_SUBST([CMOCKA_LDFLAGS], ["-L$with_cmocka/lib"])

									AX_VAR_POPVALUE([LIBS])
									AX_VAR_POPVALUE([LDFLAGS])

									# Append -I/path/to/cmocka/include to CPPFLAGS and test later
									AX_VAR_PUSHVALUE([CPPFLAGS], ["-I$with_cmocka/include"])

									AC_CHECK_HEADERS([cmocka.h],
													 [
														WITH_CMOCKA=1

														AC_SUBST([CMOCKA_CPPFLAGS],
																 ["-I$with_cmocka/include"])

														AX_VAR_POPVALUE([CPPFLAGS])
													 ],
													 [],
													 [
														# include <stdarg.h>
														# include <stddef.h>
														# include <stdint.h>
														# include <setjmp.h>
													 ])
									])
				  ])
		 AC_SUBST([HAVE_GCRYPT_H])
		 ])
