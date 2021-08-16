##*****************************************************************************
## $Id$
##*****************************************************************************
#  AUTHOR:
#    Jordi Gómez
#
#  SYNOPSIS:
#    X_AC_MPI()
#
#  DESCRIPTION:
#    Check the usual suspects for a mpi installation,
#    updating CPPFLAGS and LDFLAGS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************


AC_DEFUN([X_AC_MPI_FIND_ROOT_DIR],
[
	for d in $_x_ac_mpi_dirs_root;
	do
		for dir_midl in $_x_ac_mpi_dirs_midl;
		do
			for dir_incl in $_x_ac_mpi_dirs_incl;
			do
				test -d "$d" || continue
				test -d "$d/$dir_midl" || continue
				test -d "$d/$dir_midl/$dir_incl" || continue
				test -f "$d/$dir_midl/$dir_incl/mpi.h" || continue

				AS_VAR_SET(_cv_mpi_dir_root, $d)
				AS_VAR_SET(_cv_mpi_dir_incl, $dir_midl$dir_incl)
				test -n "$_cv_mpi_dir_root" && break
			done
			test -n "$_cv_mpi_dir_root" && break
		done
		test -n "$_cv_mpi_dir_root" && break
	done
])

AC_DEFUN([X_AC_MPI],
[
	_x_ac_mpi_dirs_root="/usr /usr/local"
	_x_ac_mpi_dirs_midl="/ /intel64/"
	_x_ac_mpi_dirs_incl="include include64"

	AC_ARG_WITH(
		[mpi],
		AS_HELP_STRING(--with-mpi=PATH,Specify path to MPI installation),
		[
			_x_ac_mpi_dirs_root="$withval"
			_x_ac_mpi_custom="yes"
		]
	)

	AC_CACHE_CHECK(
		[for MPI root directory],
		[_cv_mpi_dir_root],
		[
			X_AC_MPI_FIND_ROOT_DIR([])
			
			if test -z "$_cv_mpi_dir_root"; then
				_x_ac_mpi_dirs_root="${_ax_ld_dirs_root}"
				_x_ac_mpi_custom="yes"
				
				X_AC_MPI_FIND_ROOT_DIR([])
			fi
		]
	)

	if test -z "$_cv_mpi_dir_root"; then
		echo checking for MPI CPPFLAGS... no
	else
		MPI_DIR=$_cv_mpi_dir_root
		MPI_CPPFLAGS="-I\$MPI_BASE$_cv_mpi_dir_incl"
		echo checking for MPI CPPFLAGS... $MPI_CPPFLAGS
	fi

	AC_SUBST(MPI_CPPFLAGS)
	AC_SUBST(MPI_DIR)

	AM_CONDITIONAL(WITH_MPI, test -n "$_cv_mpi_dir_root")
])
