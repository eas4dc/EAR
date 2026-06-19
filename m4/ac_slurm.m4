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
        [AS_HELP_STRING([--with-slurm=<PATH>], [Specify path to SLURM installation (enabled by default).])],
        [],
				[with_slurm=yes]
		)

		HAVE_SLURM_H=0
		AS_IF([test "x$with_slurm" != "xno"],
					[
						dnl Check for the presence of headers
						dnl Push CPPFLAGS if a custom path is provided
						AS_IF([test "x$with_slurm" != xyes],
									[AX_VAR_PUSHVALUE([CPPFLAGS], ["-I$with_slurm/include"])])

						AC_CHECK_HEADERS([slurm/slurm.h slurm/spank.h],
														 [
						 									 ],
															 [AC_MSG_ERROR([Either slurm.h or spank.h header file not found.], [1])]
															)
						dnl If reached this section, that means headers were found.
						HAVE_SLURM_H=1
						SCHED_NAME=SLURM
						dnl If a specific slurm path was specified, set SLURM_CPPFLAGS
						dnl and restore the original CPPFLAGS variable
						AS_IF([test "x$with_slurm" != xyes],
									[
										AC_MSG_NOTICE([Found both slurm.h and spank.h at $with_slurm/include])
										AC_SUBST([SLURM_CPPFLAGS], ["-I$with_slurm/include"])
										AX_VAR_POPVALUE([CPPFLAGS])
									])
					]
				 )
	
    AC_SUBST(SLURM_CPPFLAGS)
		AC_SUBST(HAVE_SLURM_H)
])
