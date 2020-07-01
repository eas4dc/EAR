##*****************************************************************************
## $Id$
##*****************************************************************************
#  AUTHOR:
#    Jordi Gómez
#
#  DESCRIPTION:
##*****************************************************************************

AC_DEFUN([AX_PRE_OPT_FEATURES],
[
	#
	#
	#
	AC_ARG_VAR([EAR_TMP],[Defines the node local storage as 'var', 'tmp' or other tempfs file system (default: /var/ear) (you can alo use --localstatedir=DIR)])
	AC_ARG_VAR([EAR_ETC],[Defines the read-only single-machine data as 'etc' (default: EPREFIX/etc) (you can also use --sharedstatedir=DIR)])
	AC_ARG_VAR([MAN],[Defines the documentation directory (default: PREFIX/man) (you can use also --mandir=DIR)])
	AC_ARG_VAR([COEFFS],[Defines the coefficients store directory (default: EPREFIX/etc)])
	
	if test "x$prefix" = "xNONE";then
		prefix=/usr/local
	fi
	if test "x$exec_prefix" = "xNONE"; then
		exec_prefix=$prefix
	fi
	if test "x$libdir" = "x\${exec_prefix}/lib"; then
		libdir=$exec_prefix/lib
	fi
	if test "x$bindir" = "x\${exec_prefix}/bin"; then
		bindir=$exec_prefix/bin
	fi
	if test "x$sbindir" = "x\${exec_prefix}/sbin"; then
		sbindir=$exec_prefix/sbin
	fi
	if test -n "$EAR_ETC"; then
		sysconfdir=$EAR_ETC	
	fi
	if test "x$sysconfdir" = "x\${exec_prefix}/etc" || test "x$sysconfdir" = "x\${prefix}/etc"; then
		sysconfdir=$prefix/etc
	fi
	if test -n "$EAR_TMP"; then
		localstatedir=$EAR_TMP
	fi
	if test "x$localstatedir" = "x\${prefix}/var"; then
        	localstatedir=/var/ear
	fi
	if test -n "$DOC"; then
		docdir=$DOC
	fi
	if test "x$docdir" = "x\${datarootdir}/doc/\${PACKAGE_TARNAME}"; then
		docdir=/share/doc/ear
	fi

	#
	# Disable RPATH
	#
	AC_ARG_ENABLE([rpath],
		AS_HELP_STRING([--disable-rpath], [Disables RPATH/RUNPATH from the compiled binaries])
	)

	#
	# MPI
	#
	AC_ARG_VAR([CC_FLAGS],[Adds parameters to C compiler])
	AC_ARG_VAR([MPICC],[Defines the MPI compiler])
	AC_ARG_VAR([MPICC_FLAGS],[Appends parameters to MPI compiler])
	AC_ARG_VAR([MPI_VERSION],[Adds a suffix to the EAR library referring the MPI version used to compile])

	# !I && !O
	if test -z "$MPICC" && test -z "$OMPICC"; then
		MPICC=mpicc
	fi

	if echo "$CC" | grep -q "icc" && test -z "$CC_FLAGS"; then
		CC_FLAGS=-static-intel
	fi

	if echo "$MPICC" | grep -q "mpiicc" && test -z "$MPICC_FLAGS"; then	
		MPICC_FLAGS="-static-intel"
		
		if test "x$enable_rpath" = "xno"; then
			MPICC_FLAGS="$MPICC_FLAGS -norpath"
		fi
	fi
	
	if echo "$MPICC" | grep -q "/"; then
		echo nothing &> /dev/null
	else
		if which $MPICC &> /dev/null; then
			MPICC="`which $MPICC`"
		else
			MPICC="mpicaca"
		fi
	fi
	if echo "$CC" | grep -q "/"; then
		echo nothing &> /dev/null
	else
		CC="`which $CC`"
	fi

    #
    # Fortran
    #
	
	AC_ARG_WITH(
		[fortran],
		AS_HELP_STRING(--with-fortran,Adds MPI Fortran symbols.),
		[
           	enable_fort="$withval"
		]
	)
	
	FEAT_FORT=0

	if test -n "$enable_fort"; then
		FEAT_FORT=1
	fi

	#
	# Architecture
	#
	AC_ARG_ENABLE([avx512],
		AS_HELP_STRING([--disable-avx512], [Compiles replacing AVX-512 instructions by AVX-2])
        )
	
	FEAT_AVX512=1

	if test "x$enable_avx512" = "xno"; then
		FEAT_AVX512=0
	fi
	
	# AC_ARG_VAR([ARCH],[Compiles the code for a CPU architecture dis/enabling some specific features (sandy, ivy, haswell, broadwell, skylake (def))])

	# if test -z "$ARCH"; then
	# 	MARCH="skylake"
	# else
	# 	MARCH=`echo $ARCH | tr '[:upper:]' '[:lower:]'`
	# fi

	# case $MARCH in
	# 	skylake) ARCH=5;;
	# 	broadwell) ARCH=4;;
	# 	haswell) ARCH=3;;
	# 	ivy) ARCH=2;;
	# 	sandy) ARCH=1;;
	# 	*) ARCH=5;;
	# esac

 	#
	# Installation user/group
	#
	AC_ARG_VAR([USER],[Sets the owner user of your installed files])
	AC_ARG_VAR([GROUP],[Sets the owner group of your installed files])

	#
	# Other calls
	#
	#X_AC_GET_LD_LIBRARY_PATHS([])
])
