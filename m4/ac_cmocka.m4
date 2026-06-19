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
              lib_path=""
              dnl Test whether a specific root path was provided
              dnl If so, check whether the specific path exists
              AS_IF([test "x$with_cmocka" != xyes],
                    for path in "$with_cmocka/lib" "$with_cmocka/lib64"; do
                        AS_IF([test -d $path],
                              [AS_VAR_APPEND([lib_path], [$path])
                               break])
                    done
                  )
              dnl Save current LDFLAGS value and set a new search path
              dnl if one was found.
              AS_IF([test -n "$lib_path"],
                    [AC_MSG_NOTICE([Checking for $lib_path])
                     AX_VAR_PUSHVALUE([LDFLAGS], ["-L$lib_path"])])

              dnl Save current LIBS value
              AX_VAR_PUSHVALUE([LIBS],[])

              # check availability of -lcmocka
              AC_SEARCH_LIBS([cmocka_set_message_output], [cmocka],
                             [
                              AC_SUBST([CMOCKA_LIBS], [-lcmocka])
							  dnl If a specific search path was used, set CMOCKA_LDFLAGS
							  dnl and restore the original LDFLAGS variable
                              AS_IF([test -n "$lib_path"],
                              		[
                              		    AC_SUBST([CMOCKA_LDFLAGS], ["-L$lib_path"])
                              			AX_VAR_POPVALUE([LDFLAGS])
                              		]
                              	   )

							  dnl Check for the presence of headers
							  dnl Push CPPFLAGS if a custom path is provided
							  AS_IF([test "x$with_cmocka" != xyes],
							  		[AX_VAR_PUSHVALUE([CPPFLAGS], ["-I$with_cmocka/include"])])

                              AC_CHECK_HEADERS([cmocka.h],
                              [
                               WITH_CMOCKA=1

							   dnl If a specific cmocka path was specified, set CMOCKA_CPPFLAGS
							   dnl and restore the original CPPFLAGS variable
							   AS_IF([test "x$with_cmocka" != xyes],
                                     [
                                      AC_MSG_NOTICE([Found cmocka.h at $with_cmocka/include])
                                      AC_SUBST([CMOCKA_CPPFLAGS], ["-I$with_cmocka/include"])
                                      AX_VAR_POPVALUE([CPPFLAGS])
									 ])
                              ],
                              [AC_MSG_ERROR([cmocka.h header not found.], [1])],
                              [
                               # include <stdarg.h>
                               # include <stddef.h>
                               # include <stdint.h>
                               # include <setjmp.h>
                              ])
                             ],
                             [AC_MSG_ERROR([libcmocka not found although it was requested.], [1])])
              AX_VAR_POPVALUE([LIBS]) dnl restore the original value
              ])
        AC_SUBST([WITH_CMOCKA])
])
