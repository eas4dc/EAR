# INCLUDES
m4_include([m4/ax_compare_version.m4])
m4_include([m4/ax_architectures.m4])
m4_include([m4/ax_features.m4])
m4_include([m4/ax_defaults.m4])
m4_include([m4/ac_freeipmi.m4])
m4_include([m4/ac_redfish.m4])
m4_include([m4/ac_likwid.m4])
m4_include([m4/ac_oneapi.m4])
m4_include([m4/ac_pgsql.m4])
m4_include([m4/ac_mysql.m4])
m4_include([m4/ac_slurm.m4])
m4_include([m4/ac_cupti.m4])
m4_include([m4/ac_cuda.m4])
m4_include([m4/ac_rsmi.m4])
m4_include([m4/ac_pbs.m4])
m4_include([m4/ac_oar.m4])
m4_include([m4/ac_gsl.m4])
m4_include([m4/ac_dlb.m4])
m4_include([m4/ac_countdown.m4])

# INIT
AC_PREREQ([2.69])
AC_INIT([EAR], [5.2], [ear-support@bsc.es], [ear-5.2], [https://gitlab.bsc.es/ear_team/ear])
AC_COPYRIGHT([Copyright © 2017-present BSC])
AC_LANG(C)

# PROGRAMS TEST
AC_PROG_CC
AC_PROG_AWK
AC_PROG_GREP
AC_PROG_MAKE_SET

#################
# FIRST SCRIPTS #
#################
#

AX_SET_DEFAULTS
AX_SET_ARCHITECTURES
AX_BEFORE_FEATURES

############
# FEATURES #
############

X_AC_CUDA
X_AC_CUPTI
X_AC_RSMI
X_AC_ONEAPI
X_AC_LIKWID
X_AC_FREEIPMI
X_AC_GSL
X_AC_PGSQL
X_AC_MYSQL
X_AC_ONEAPI
X_AC_REDFISH
X_AC_SLURM
X_AC_OAR
X_AC_PBS
X_AC_DLB
X_AC_COUNTDOWN

#################
# POST-FEATURES #
#################

AX_AFTER_FEATURES

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

AS_ECHO("--------------------------------------- Checking summary --")
AC_DEPENDANCY_CHECK($MPI_DIR, MPI found, 0)
AC_DEPENDANCY_CHECK($GSL_DIR, GSL found, 1)
AC_DEPENDANCY_CHECK($SCHED_DIR, SLURM/PBS/OAR found ($SCHED_NAME), 0)
AC_DEPENDANCY_CHECK($DB_DIR, Database found ($DB_NAME), 1)
AS_ECHO("----------------------------------------------------------")


#if test -z $COMPILABLE; then
#	AC_MSG_ERROR(not compilable)
#fi

AC_CONFIG_FILES([Makefile
								 src/common/config/config_install.h
                 etc/module/ear
                 etc/module/ear-lite
                 etc/module/ear-sing
                 etc/module/ear-debug
                 etc/slurm/ear.plugstack.conf
                 etc/systemd/eard.service
                 etc/systemd/eardbd.service
                 etc/systemd/eargmd.service
                 etc/systemd/edcmon.service
                 etc/conf/ear.conf.full.template
                 etc/conf/ear.conf.template
                 etc/rpms/specs/ear.spec
				 etc/rpms/specs/ear_ln.spec
				 etc/rpms/specs/ear_cn.spec
				 etc/rpms/specs/ear_sn.spec])
AC_CONFIG_FILES([etc/rpms/rpmbuild.sh], [chmod +x etc/rpms/rpmbuild.sh])
AC_OUTPUT
AX_AFTER_OUTPUT
