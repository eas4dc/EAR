
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
#    Lluís Alonso
#
#  SYNOPSIS:
#    X_AC_OPENSSL()
#
#  DESCRIPTION:
#    Check the usual suspects for a openssl installation,
#    updating CPPFLAGS and LDFLAGS as necessary.
#
#  WARNINGS:
#    This macro must be placed after AC_PROG_CC and before AC_PROG_LIBTOOL.
##*****************************************************************************

AC_DEFUN([X_AC_OPENSSL_FIND_ROOT_DIR],
[
    for d in $_x_ac_openssl_dirs_root; do
        test -d "$d" || continue
        test -d "$d/include" || continue
        test -d "$d/include/openssl" || continue
        test -f "$d/include/openssl/err.h" || continue
        test -f "$d/include/openssl/evp.h" || continue

        AS_VAR_SET(_cv_openssl_dir_root, $d)
        break
    done
])

AC_DEFUN([X_AC_OPENSSL],
[
    _x_ac_openssl_is_active=0
    _x_ac_openssl_dirs_root="/usr /usr/local /opt"
    _cv_openssl_dir_root=

    #check if the argument is called
    AC_ARG_WITH([openssl],
        AS_HELP_STRING(--with-openssl=OPTIONAL_PATH,
                        [Specify to enable OpenSSL encryption. If no path is given, the default directories will be checked.]
                       ),
        [
            _x_openssl_base="$withval"

            #if it's --openssl=path, we substitute the search path
            if test "$withval" != "yes"; then
                _x_ac_openssl_dirs_root="$withval"
            fi

            #openssl is only active if configure is called with --with-openssl
            _x_ac_openssl_is_active=1
        ]
    )

    OPENSSL_BASE=
    OPENSSL_CONSTANT=
    OPENSSL_LDFLAGS=

    if test "$_x_ac_openssl_is_active" = "1"; then
       #search if the files exist in the path, if they do set the variables
       X_AC_OPENSSL_FIND_ROOT_DIR([])
       if test -z "$_cv_openssl_dir_root"; then
          echo checking for OPENSSL compiler link... no
       else
          echo checking for OPENSSL compiler link... yes
          OPENSSL_CONSTANT="-DOPENSSL_SUPPORT=1"
          OPENSSL_BASE=$_cv_openssl_dir_root
          OPENSSL_LDFLAGS="-lcrypto"
       fi
    fi

    AC_SUBST(OPENSSL_CONSTANT)
    AC_SUBST(OPENSSL_BASE)
    AC_SUBST(OPENSSL_LDFLAGS)
])
