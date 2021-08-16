# INCLUDES
m4_include([m4/ax_compare_version.m4])
m4_include([m4/ax_features.m4])
m4_include([m4/x_ac_pgsql.m4])
m4_include([m4/x_ac_mysql.m4])
m4_include([m4/x_ac_slurm.m4])
m4_include([m4/x_ac_cuda.m4])
m4_include([m4/x_ac_gsl.m4])

# INIT
AC_PREREQ([2.69])
AC_INIT([EAR], [4.0])
AC_LANG(C)

# PROGRAMS TEST
AC_PROG_CC
AC_PROG_AWK
AC_PROG_GREP

################
# PRE-FEATURES #
################
#

AX_PRE_OPT_FEATURES

############
# FEATURES #
############
#

############
# GSL TEST #
############
#
# Installation directories:
#   --prefix=PREFIX         install architecture-independent files in PREFIX [/usr/local]
#   --exec-prefix=EPREFIX   install architecture-dependent files in EPREFIX [PREFIX]
#
# By default, `make install' will install all the files in
# `/usr/local/bin', `/usr/local/lib' etc.  You can specify
# an installation prefix other than `/usr/local' using `--prefix',
# for instance `--prefix=$HOME'.
#
# For better control, use the options below.
#
# Fine tuning of the installation directories:
#   --bindir=DIR            user executables [EPREFIX/bin]
#   --sbindir=DIR           system admin executables [EPREFIX/sbin]
#   --libdir=DIR            object code libraries [EPREFIX/lib]
#   --includedir=DIR        C header files [PREFIX/include]
#

X_AC_GSL

#########
# SLURM #
#########
# Installation directories:
#     --prefix=PREFIX         install architecture-independent files in PREFIX [/usr/local]
#     --exec-prefix=EPREFIX   install architecture-dependent files in EPREFIX [PREFIX]
#
#   By default, `make install' will install all the files in
#   `/usr/local/bin', `/usr/local/lib' etc.  You can specify
#   an installation prefix other than `/usr/local' using `--prefix',
#   for instance `--prefix=$HOME'.
#
#   For better control, use the options below.
#
#   Fine tuning of the installation directories:
#     --bindir=DIR            user executables [EPREFIX/bin]
#     --sbindir=DIR           system admin executables [EPREFIX/sbin]
#     --libdir=DIR            object code libraries [EPREFIX/lib]
#     --includedir=DIR        C header files [PREFIX/include]
#

X_AC_SLURM

#########
# PGSQL #
#########
#

X_AC_PGSQL

#########
# MYSQL #
#########
#

X_AC_MYSQL

#########
# CUDA  #
#########
#

X_AC_CUDA

#################
# POST-FEATURES #
#################
#

AX_POST_OPT_FEATURES

##########
# OUTPUT #
##########

COMPILABLE="yes"

AC_DEFUN([AC_DEPENDANCY_CHECK],
[
	AC_MSG_CHECKING($2)

	if test -n "$1"; then
		AC_MSG_RESULT(yes)
	else
		if test $3 == 1; then
			COMPILABLE=""
		fi
		AC_MSG_RESULT(no)
	fi
])

AC_DEFUN([AC_FEATURE_CHECK],
[
 	if test "x$1" = "x$2"; then
        AC_MSG_RESULT(Enabled $3 feature... yes)
    else    
        AC_MSG_RESULT(Enabled $3 feature... no)
    fi
])

echo "---------------------------------------Checking summary --"
AC_DEPENDANCY_CHECK($GSL_DIR, GSL dependancy found, 1)
AC_DEPENDANCY_CHECK($SLURM_DIR, SLURM dependancy found, 0)
AC_DEPENDANCY_CHECK($DB_DIR, Database dependancy found ($DB_NAME), 1)
echo "----------------------------------------------------------"

#if test -z $COMPILABLE; then
#	AC_MSG_ERROR(not compilable)
#fi

AC_CONFIG_FILES([Makefile
                 etc/module/ear
                 etc/slurm/ear.plugstack.conf
                 etc/systemd/eard.service
                 etc/systemd/eardbd.service
                 etc/systemd/eargmd.service
                 src/common/config/config_install.h
                 src/common/config/config_dev.h
                 src/common/config/config_def.h
                 src/common/config/config_env.h
                 etc/conf/ear.conf.full.template
                 etc/conf/ear.conf.template
                 etc/rpms/specs/ear.spec])
AC_CONFIG_FILES([etc/rpms/rpmbuild.sh], [chmod +x etc/rpms/rpmbuild.sh])
AC_OUTPUT
