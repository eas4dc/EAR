##*****************************************************************************
## $Id$
##*****************************************************************************
#  AUTHOR:
#    Jordi Gómez
#
#  DESCRIPTION:
##*****************************************************************************

AC_DEFUN([AX_POST_OPT_FEATURES],
[
	if test "x$DB_NAME" = "xpgsql"; then
		DB_MYSQL=0
		DB_PGSQL=1
	else
		DB_MYSQL=1
		DB_PGSQL=0
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

	AC_SUBST(EAR_TMP)
	AC_SUBST(MPICC)
	AC_SUBST(CC_FLAGS)
	AC_SUBST(MPICC_FLAGS)
	AC_SUBST(MPI_VERSION)
	AC_SUBST(FEAT_FORT)
	AC_SUBST(FEAT_AVX512)
	AC_SUBST(DB_MYSQL)
	AC_SUBST(DB_PGSQL)
	AC_SUBST(VERSION_MAJOR)
	AC_SUBST(VERSION_MINOR)
])
