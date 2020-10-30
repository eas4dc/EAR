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
#    updating CPPFLAGS and LDFLAGS as necessary.
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

	if test $FEAT_GPUS = 1;
	then
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
	fi

	if test -z "$_cv_cuda_dir_root"; then
		echo checking for CUDA CPPFLAGS... no
		FEAT_GPUS=0
	else
		CUDA_DIR=$_cv_cuda_dir_root
		CUDA_CPPFLAGS="-I$CUDA_DIR/include"
		echo checking for CUDA CPPFLAGS... $CUDA_CPPFLAGS
	fi

	AC_SUBST(CUDA_CPPFLAGS)
	AC_SUBST(CUDA_DIR)

	AM_CONDITIONAL(WITH_CUDA, test -n "$_cv_cuda_dir_root")
])
