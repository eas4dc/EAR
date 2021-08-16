# Commands
# -i <pack.file>  installs package
# -e <pack.name>  uninstall package
# --prefix <path>   sets the installation path
# --test      don't install, but tell if it would work or not
# -qpl        query + package + list, list all the files in RPM

# Var definitions
%define __requires_exclude libmpi*.*$|libpapi.so.*|libgsl*

%define name    ear
%define release   4
%define version   4.0

# Information
# Prefix  Just for when missing any prefix
Summary:  EAR package
Group:    System
Packager: EAR Team
URL:    https://github.com/BarcelonaSupercomputingCenter/ear_private
License:  GPL 2.1
Name:   %{name}
Version:  %{version}%{?dist}
Release:  %{release}
Source:   %{name}‑%{version}.tar.gz
Buildroot:  %{_topdir}/BUILDROOT
Prefix:   /usr
Prefix: /etc

# Brief description of the project
%description
EAR RPM package includes the daemons, configuration files
and tools required to make EAR works in your cluster.

# RPMBuild context
%prep
mkdir -p  %{buildroot}/
mkdir -p  %{buildroot}/usr/include
mkdir -p  %{buildroot}/usr/lib
mkdir -p  %{buildroot}/usr/lib/plugins
mkdir -p  %{buildroot}/usr/sbin
mkdir -p  %{buildroot}/usr/bin
mkdir -p  %{buildroot}/usr/bin/tools
mkdir -p  %{buildroot}/etc/ear
mkdir -p  %{buildroot}/etc/ear/coeffs
mkdir -p  %{buildroot}/etc/systemd/system
mkdir -p  %{buildroot}/etc/module
mkdir -p  %{buildroot}/etc/slurm
mkdir -p  %{buildroot}/var/ear
cp    -rp ${EAR_INSTALL_PATH}/bin/tools/* %{buildroot}/usr/bin/tools/
cp    -rp ${EAR_INSTALL_PATH}/bin/eacct %{buildroot}/usr/bin/
cp    -rp ${EAR_INSTALL_PATH}/bin/ereport %{buildroot}/usr/bin/
cp    -rp ${EAR_INSTALL_PATH}/bin/econtrol %{buildroot}/usr/bin/
cp    -rp ${EAR_INSTALL_PATH}/include/ear.h %{buildroot}/usr/include/
cp    -p  ${EAR_SOURCE_PATH}/etc/module/ear.in %{buildroot}/etc/module/
touch %{buildroot}/etc/module/ear
cp    -p  ${EAR_SOURCE_PATH}/etc/conf/ear.conf.template.in %{buildroot}/etc/ear/ear.conf.template.in
cp    -p  ${EAR_SOURCE_PATH}/etc/conf/ear.conf.full.template.in %{buildroot}/etc/ear/ear.conf.full.template.in
touch %{buildroot}/etc/ear/ear.conf.template
touch %{buildroot}/etc/ear/ear.conf.full.template
cp  -p  ${EAR_SOURCE_PATH}/etc/slurm/ear.plugstack.conf.in %{buildroot}/etc/slurm/
touch %{buildroot}/etc/slurm/ear.plugstack.conf
cp	-rp ${EAR_INSTALL_PATH}/lib/libear.* %{buildroot}/usr/lib/
cp	-rp ${EAR_INSTALL_PATH}/lib/libearld.* %{buildroot}/usr/lib/
cp    -rp ${EAR_INSTALL_PATH}/lib/libear_api.a %{buildroot}/usr/lib/libear_api.a
cp    -rp ${EAR_INSTALL_PATH}/lib/plugins/* %{buildroot}/usr/lib/plugins/
cp	-rp ${EAR_INSTALL_PATH}/lib/earplug.so %{buildroot}/usr/lib/
cp	-rp ${EAR_INSTALL_PATH}/bin/erun %{buildroot}/usr/bin/
cp  -p  ${EAR_SOURCE_PATH}/etc/systemd/eargmd.service.in %{buildroot}/etc/systemd/system
cp  -p  ${EAR_SOURCE_PATH}/etc/systemd/eardbd.service.in %{buildroot}/etc/systemd/system
cp  -p  ${EAR_SOURCE_PATH}/etc/systemd/eard.service.in %{buildroot}/etc/systemd/system
touch %{buildroot}/etc/systemd/system/eargmd.service
touch %{buildroot}/etc/systemd/system/eard.service
touch %{buildroot}/etc/systemd/system/eardbd.service
cp	-rp ${EAR_INSTALL_PATH}/sbin/eargmd %{buildroot}/usr/sbin/
cp	-rp ${EAR_INSTALL_PATH}/sbin/eardbd %{buildroot}/usr/sbin/
cp	-rp ${EAR_INSTALL_PATH}/sbin/eard %{buildroot}/usr/sbin/
cp	-rp ${EAR_INSTALL_PATH}/sbin/edb_create %{buildroot}/usr/sbin/
cp   -p  ${EAR_SOURCE_PATH}/etc/rpms/configure/configure %{buildroot}/etc/

exit

# RPMBuild context
%files
%attr(-, -, -) /usr/*
%attr(-, -, -) /etc/*

#Comments for change log
# * [dow mon dd yyyy] [packager [email address]] [RPM version]-list of changes
%changelog
* Tue Mar 31 2020 Julita Corbalan <julita.corbalan@bsc.es> 3.2
- GPU, PCK and DRAM reported in DB
- USE_GPU set to 1
* Tue Feb 11 2020 Julita Corbalan <julita.corbalan@bsc.es> 3.1.3
- Fixed error when reading GPU energy
- Fixed error when resetting IPMI interface
* Wed Jan 16 2020 Julita Corbalan <julita.corbalan@bsc.es> 3.1.2
- init_power_monitoring in dynamic_configuration
- gpu support included
- new power_models or avx512
* Mon Dec 09 2019 Julita Corbalan <julita.corbalan@bsc.es> 3.1.1
- energy_to_str function in energy_rapl
- msr read and write with do_while loop
* Thu Dec 05 2019 Julita Corbalan <julita.corbalan@bsc.es> 3.1.1
- Policy privilege specification modified from Y/N to 0/1
* Thu Nov 28 2019 Julita Corbalan <julita.corbalan@bsc.es> 3.1.0
-Plugins for energy, power policies, power models and traces
* Thu Nov 28 2019 Julita Corbalan <julita.corbalan@bsc.es> 3.1.0
-ear.conf policy definition modified
* Thu Nov 28 2019 Julita Corbalan <julita.corbalan@bsc.es> 3.1.0
-Support for multiple EAR library versions more automatic. SLURM_EAR_MPI_VERSION for automatic selection.
* Thu Nov 28 2019 Julita Corbalan <julita.corbalan@bsc.es> 3.1.0
-Support for N sockets per node
* Thu Nov 28 2019 Julita Corbalan <julita.corbalan@bsc.es> 3.1.0
-Support for Multiple profiling libraries using LD_PRELOAD mechanism
* Thu Nov 28 2019 Julita Corbalan <julita.corbalan@bsc.es> 3.1.0
-CPUpower nd Freeipmi dependency removed
* Thu Nov 28 2019 Julita Corbalan <julita.corbalan@bsc.es> 3.1.0
-Temperature reported with node_metrics. Not in DB

# RPMBuild context
%clean
rm -rf %{_topdir}/BUILD
rm -rf %{_topdir}/BUILDROOT
rm -rf %{_topdir}/SOURCES
rm -rf %{_topdir}/SPECS
rm -rf %{_topdir}/SRPMS

# RPM context
%post
(cd ${RPM_INSTALL_PREFIX1} && ./configure --prefix=${RPM_INSTALL_PREFIX0} --sysconfdir=${RPM_INSTALL_PREFIX1} )
rm    -f ${RPM_INSTALL_PREFIX1}/config.log
rm    -f ${RPM_INSTALL_PREFIX1}/config.status



