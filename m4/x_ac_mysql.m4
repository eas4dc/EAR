##*****************************************************************************
## $Id$
##*****************************************************************************
#  AUTHOR:
#    Jordi Gómez
#
#  SYNOPSIS:
#    X_AC_MYSQL()
#
#  DESCRIPTION:
#    Check the usual suspects for a mysql installation,
#    updating CPPFLAGS and LDFLAGS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************

AC_DEFUN([X_AC_MYSQL_FIND_ROOT_DIR],
[
	for d in $_x_ac_mysql_dirs_root; do
		test -d "$d" || continue
		test -d "$d/include" || continue
		test -d "$d/include/mysql" || continue
		test -f "$d/include/mysql/mysql.h" || continue
		for dir_lib in $_x_ac_mysql_dirs_libs;
		do
			# Testing if the library folder exists
			test -d $d/$dir_lib || continue

			# If exists, then its path and LDFLAGS are saved
			if test -d "$d/$dir_lib"; then
				_x_ac_mysql_dir_lib="$d/$dir_lib"
				_x_ac_mysql_gcc_ldflags=-L$_x_ac_mysql_dir_lib
			fi

			X_AC_VAR_BACKUP([],[$_x_ac_mysql_gcc_ldflags],[$_x_ac_mysql_gcc_libs])
			#
			# AC_LANG_CALL(prologue, function) creates a source file
			# and calls to the function.
			# AC_LINK_IFELSE(input, action-if-true) runs the compiler
			# and linker using LDFLAGS and LIBS.
			#
			AC_LINK_IFELSE(
				[AC_LANG_CALL([], mysql_server_end)],
				[mysql_server_end=yes]
			)
			X_AC_VAR_UNBACKUP

			if test "x$mysql_server_end" = "xyes"; then
				AS_VAR_SET(_cv_mysql_dir_root, $d)
			fi

			test -n "$_cv_mysql_dir_root" && break
		done
		test -n "$_cv_mysql_dir_root" && break
	done
])

AC_DEFUN([X_AC_MYSQL],
[
	_x_ac_mysql_dirs_root="/usr /usr/local /opt"
	_x_ac_mysql_dirs_libs="lib64/mysql lib/mysql lib64"
    _x_ac_mysql_gcc_libs="-lmysqlclient"
    _x_ac_mysql_gcc_ldflags=
    _x_ac_mysql_dir_bin=
    _x_ac_mysql_dir_lib=

	AC_ARG_WITH(
		[mysql],
		AS_HELP_STRING(--with-mysql=PATH,Specify path to MySQL installation),
        	[
			_x_ac_mysql_dirs_root="$withval"
			_x_ac_mysql_custom="yes"
			_x_ac_mysql_with="yes"
		]
	)

	AC_CACHE_CHECK(
        [for MySQL root directory],
        [_cv_mysql_dir_root],
        [
			X_AC_MYSQL_FIND_ROOT_DIR([])
			if test -z "$_cv_mysql_dir_root"; then
			_x_ac_mysql_dirs_root="${_ax_ld_dirs_root}"
			_x_ac_mysql_custom="yes"
			X_AC_MYSQL_FIND_ROOT_DIR([])
			fi
        ]
    )

	if test -z "$_cv_mysql_dir_root"; then
		echo checking for MySQL compiler link... no
	else
		DB_NAME=mysql
		DB_DIR=$_cv_mysql_dir_root
		DB_LIBS=$_x_ac_mysql_gcc_libs
		DB_LDFLAGS=$_x_ac_mysql_gcc_ldflags

		if test "x$_x_ac_mysql_custom" = "xyes"; then
			DB_LIBDIR=$_x_ac_mysql_dir_lib
			DB_CPPFLAGS="-I$DB_DIR/include"
		fi

		echo checking for MySQL compiler link... yes
		echo checking for MySQL CPPFLAGS... $DB_CPPFLAGS
		echo checking for MySQL LDFLAGS... $DB_LDFLAGS
		echo checking for MySQL libraries... $DB_LIBS
	fi

    AC_SUBST(DB_LIBS)
    AC_SUBST(DB_LIBDIR)
    AC_SUBST(DB_CPPFLAGS)
    AC_SUBST(DB_LDFLAGS)
    AC_SUBST(DB_DIR)

    AM_CONDITIONAL(WITH_MYSQL, test -n "$_cv_mysql_dir_root")
])
