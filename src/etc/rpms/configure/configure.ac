AC_PREREQ([2.69])
AC_INIT([EAR], [3.0])

if test "x$sysconfdir" = "x\${exec_prefix}/etc" || test "x$sysconfdir" = "x\${prefix}/etc"; then
	sysconfdir=$prefix/etc
fi
if test "x$localstatedir" = "x\${prefix}/var"; then
	localstatedir=/var/ear
fi


AC_CONFIG_FILES([
	${sysconfdir}/ear/ear.conf.template
	${sysconfdir}/ear/ear.conf.full.template
	${sysconfdir}/module/ear
	${sysconfdir}/systemd/system/eard.service
	${sysconfdir}/systemd/system/eardbd.service
	${sysconfdir}/systemd/system/eargmd.service
	${sysconfdir}/slurm/ear.plugstack.conf])
AC_OUTPUT
