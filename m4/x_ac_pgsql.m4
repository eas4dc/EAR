##*****************************************************************************
## $Id$
##*****************************************************************************
#  AUTHOR:
#    Jordi Gómez
#
#  SYNOPSIS:
#    X_AC_PGSQL()
#
#  DESCRIPTION:
#    Check the usual suspects for a postgresql installation,
#    updating CPPFLAGS and LDFLAGS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************

AC_DEFUN([X_AC_PGSQL_FIND_ROOT_DIR],
[
	for d in $_x_ac_pgsql_dirs_root; do
		test -d "$d" || continue
		test -d "$d/include" || continue
		test -f "$d/include/libpq-fe.h" || continue
		for dir_lib in $_x_ac_pgsql_dirs_libs;
		do
			# Testing if the library folder exists
			test -d $d/$dir_lib || continue

			# If exists, then its path and LDFLAGS are saved
			if test -d "$d/$dir_lib"; then
				_x_ac_pgsql_dir_lib="$d/$dir_lib"
				_x_ac_pgsql_gcc_ldflags=-L$_x_ac_pgsql_dir_lib
			fi

			X_AC_VAR_BACKUP([],[$_x_ac_pgsql_gcc_ldflags],[$_x_ac_pgsql_gcc_libs])
			#
			# AC_LANG_CALL(prologue, function) creates a source file
			# and calls to the function.
			# AC_LINK_IFELSE(input, action-if-true) runs the compiler
			# and linker using LDFLAGS and LIBS.
			#
			AC_LINK_IFELSE(
				[AC_LANG_CALL([], PQconndefaults)],
				[pgsql_server_con=yes]
			)
			X_AC_VAR_UNBACKUP

			if test "x$pgsql_server_con" = "xyes"; then
				AS_VAR_SET(_cv_pgsql_dir_root, $d)
			fi

			test -n "$_cv_pgsql_dir_root" && break
		done
		test -n "$_cv_pgsql_dir_root" && break
	done
])

AC_DEFUN([X_AC_PGSQL],
[
	_x_ac_pgsql_dirs_root="/usr /usr/local /opt"
	_x_ac_pgsql_dirs_libs="lib64 lib"
	_x_ac_pgsql_gcc_libs="-lpq"
	_x_ac_pgsql_gcc_ldflags=
	_x_ac_pgsql_dir_bin=
	_x_ac_pgsql_dir_lib=

	AC_ARG_WITH(
        	[pgsql],
        AS_HELP_STRING(--with-pgsql=PATH,Specify path to PostgreSQL installation),
        	[
			_x_ac_pgsql_dirs_root="$withval"
			_x_ac_pgsql_custom="yes"
		]
	)
	
	AS_IF([test "x$_x_ac_mysql_with" != "xyes"],
	[
    		AC_CACHE_CHECK(
			[for PostgreSQL root directory],
        		[_cv_pgsql_dir_root],
        		[
				X_AC_PGSQL_FIND_ROOT_DIR([])
				if test -z "$_cv_pgsql_dir_root"; then
				_x_ac_pgsql_dirs_root="${_ax_ld_dirs_root}"
				_x_ac_pgsql_custom="yes"
				X_AC_PGSQL_FIND_ROOT_DIR([])
				fi
			]
		)

		if test -z "$_cv_pgsql_dir_root"; then
			echo checking for PostgreSQL compiler link... no
		else
			DB_NAME=pgsql
			DB_DIR=$_cv_pgsql_dir_root
			DB_LIBS=$_x_ac_pgsql_gcc_libs
			DB_LDFLAGS=$_x_ac_pgsql_gcc_ldflags
		
		if test "x$_x_ac_pgsql_custom" = "xyes"; then
			DB_LIBDIR=$_x_ac_pgsql_dir_lib
			DB_CPPFLAGS="-I$DB_DIR/include"
		fi

		echo checking for PostgreSQL compiler link... yes
		echo checking for PostgreSQL CPPFLAGS... $DB_CPPFLAGS
		echo checking for PostgreSQL LDFLAGS... $DB_LDFLAGS
		echo checking for PostgreSQL libraries... $DB_LIBS
    		fi
	])

	AC_SUBST(DB_LIBS)
	AC_SUBST(DB_LIBDIR)
	AC_SUBST(DB_CPPFLAGS)
	AC_SUBST(DB_LDFLAGS)
	AC_SUBST(DB_DIR)

	AM_CONDITIONAL(WITH_PGSQL, test -n "$_cv_pgsql_dir_root")
])
