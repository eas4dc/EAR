############################################################################
# Copyright (c) 2024 Energy Aware Runtime - Barcelona Supercomputing Center
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0
############################################################################
AC_DEFUN([X_AC_VAR_BACKUP], [
    CPPFLAGS_backup=$CPPFLAGS
    CPPFLAGS=$1
    LDFLAGS_backup=$LDFLAGS
    LDFLAGS=$2
    LIBS_backup=$LIBS
    LIBS=$3
])

AC_DEFUN([X_AC_VAR_UNBACKUP], [
    CPPFLAGS=$CPPFLAGS_backup
    LDFLAGS=$LDFLAGS_backup
    LIBS=$LIBS_backup
])

AC_DEFUN([X_AC_VAR_PRINT], [
    echo $CPPFLAGS
    echo $LDFLAGS
    echo $LIBS
])

AC_DEFUN([X_AC_GET_LD_LIBRARY_PATHS], [
    LIST_LD=`echo $LD_LIBRARY_PATH | tr ':' ' '`
    _ax_ld_dirs_root=
    for p in ${LIST_LD}; do
        _ax_ld_dirs_root="${_ax_ld_dirs_root=} `echo $(cd ${p}/..; pwd)`"
    done
])

AC_DEFUN([AX_BEFORE_FEATURES],
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
		AS_HELP_STRING([--disable-rpath], [Disables RPATH/RUNPATH from the compiled binaries]))

	#
	# MPI
	#
    AC_ARG_ENABLE([mpi],
        AS_HELP_STRING([--disable-mpi], [Compiles the non-mpi version of the library.]))
    AC_ARG_ENABLE([mpi-loader],
        AS_HELP_STRING([--disable-mpi-loader], [Compiles the non-mpi version of the loader.]))

	AC_ARG_VAR([CC_FLAGS],[Adds parameters to C compiler])
	AC_ARG_VAR([MPICC],[Defines the MPI compiler])
	AC_ARG_VAR([MPICC_FLAGS],[Appends parameters to MPI compiler])
	AC_ARG_VAR([MPI_VERSION],[Adds a suffix to the EAR library referring the MPI version used to compile])

    FEAT_MPI_LIBRARY=1
    FEAT_MPI_LOADER=1

	if test -z "$MPICC"; then
		if which mpicc &> /dev/null; then
			MPICC="`which mpicc`"
		else
            FEAT_MPI_LIBRARY=0
            FEAT_MPI_LOADER=0
		fi
    else
		if which $MPICC &> /dev/null; then
			MPICC="`which $MPICC`"
        fi
	fi

	# The disable flag has the priority, so it is computed at the end
    if test "x$enable_mpi" = "xno"; then
        FEAT_MPI_LIBRARY=0
    fi
    if test "x$enable_mpi_loader" = "xno"; then
        FEAT_MPI_LOADER=0
    fi

	if echo "$CC" | grep -q "/"; then
		echo nothing &> /dev/null
	else
		CC="`which $CC`"
	fi

	MPI_DIR=`dirname $MPICC`
	MPI_DIR=`(cd $MPI_DIR/.. && pwd)`
	MPI_CFLAGS="-I\$(MPI_BASE)/include"

    if test -z "$MPICC"; then
        echo checking for MPI compiler... no
    else
        echo checking for MPI compiler... yes
        echo checking for MPI CFLAGS... -I$MPI_DIR/include
    fi

	#
	# Architecture
	#
	AC_ARG_ENABLE([avx512],
		AS_HELP_STRING([--disable-avx512], [Compiles replacing AVX-512 instructions by AVX-2]))
	
	FEAT_AVX512=1

	if test "x$enable_avx512" = "xno"; then
		FEAT_AVX512=0
	fi

	# GPUs
	
	AC_ARG_ENABLE([gpus],
		AS_HELP_STRING([--disable-gpus], [Does not allocate GPU types nor report any information]))
	
	FEAT_GPUS=1
	if test "x$enable_gpus" = "xno"; then
		FEAT_GPUS=0
	fi
	
 	#
	# Installation user/group
	#
	AC_ARG_VAR([USER],[Sets the owner user of your installed files])
	AC_ARG_VAR([GROUP],[Sets the owner group of your installed files])

    #
    # Makefile name
    #
	AC_ARG_VAR([MAKE_NAME],[Add a name to the compilation. It adds an additional Makefile with a suffix.])

   	#
   	# System type: Numbers defined in config_def.h
   	#
   	SYSTEM_TYPE=100
   	if test "x$system_type" = "xLenovo"; then
			SYSTEM_TYPE=0
		fi
   	if test "x$system_type" = "xGrace_Hooper"; then
			SYSTEM_TYPE=1
		fi
   	if test "x$system_type" = "xCray_HPE"; then
			SYSTEM_TYPE=2
		fi
   	if test "x$system_type" = "xEAR_Node_Mgr"; then
			SYSTEM_TYPE=3
		fi
		
])

AC_DEFUN([AX_AFTER_FEATURES],
[
	if test "x$DB_NAME" = "xpgsql"; then
		FEAT_DB_MYSQL=0
		FEAT_DB_PGSQL=1
	else
	    if test "x$DB_NAME" = "xmysql"; then
		    FEAT_DB_MYSQL=1
		    FEAT_DB_PGSQL=0
	    else
		    FEAT_DB_MYSQL=0
		    FEAT_DB_PGSQL=0
        fi
	fi

    # Default
    FEAT_SCHED_SLURM=0
    FEAT_SCHED_PBS=0
    FEAT_SCHED_OAR=0

    if test "x$SCHED_NAME" = "xSLURM"; then
        FEAT_SCHED_SLURM=1
        FEAT_SCHED_PBS=0
        FEAT_SCHED_OAR=0
    fi
    if test "x$SCHED_NAME" = "xPBS"; then
        FEAT_SCHED_SLURM=0
        FEAT_SCHED_PBS=1
        FEAT_SCHED_OAR=0
    fi
    if test "x$SCHED_NAME" = "xOAR"; then
        FEAT_SCHED_SLURM=0
        FEAT_SCHED_PBS=0
        FEAT_SCHED_OAR=1
    fi

	#IFS='.' read -r -a array_version <<< "$PACKAGE_VERSION" && echo $array_version[0]
	#column -t -s '\ $PACKAGE_VERSION
	#echo ${PACKAGE_VERSION} | column -t -s '.' | awk '{print $1}'
	
	OIFS=$IFS
	IFS='.'
	for x in $PACKAGE_VERSION; do
    	VERSION_MAJOR="[$x]"
		break
	done
	for x in $PACKAGE_VERSION; do
    	VERSION_MINOR="[$x]"
	done
	IFS=$OIFS

	#
	#
	#

	AC_SUBST(CC_FLAGS)
	AC_SUBST(MPICC)
	AC_SUBST(MPICC_FLAGS)
	AC_SUBST(MPI_DIR)
	AC_SUBST(MPI_CFLAGS)
	AC_SUBST(MPI_VERSION)
	AC_SUBST(FEAT_MPI_LIBRARY)
	AC_SUBST(FEAT_MPI_LOADER)
	AC_SUBST(FEAT_AVX512)
	AC_SUBST(FEAT_GPUS)
	AC_SUBST(FEAT_DB_MYSQL)
	AC_SUBST(FEAT_DB_PGSQL)
	AC_SUBST(FEAT_SCHED_SLURM)
	AC_SUBST(FEAT_SCHED_PBS)
    AC_SUBST(FEAT_SCHED_OAR)
	AC_SUBST(EAR_TMP)
	AC_SUBST(VERSION_MAJOR)
	AC_SUBST(VERSION_MINOR)
    AC_SUBST(SCHED_DIR)
    AC_SUBST(SCHED_NAME)
	AC_SUBST(SYSTEM_TYPE)
])

AC_DEFUN([AX_AFTER_OUTPUT],
[
	if test -n "$MAKE_NAME"; then
        cp Makefile Makefile.$MAKE_NAME
	fi
])
