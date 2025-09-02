# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- v6.0 GPU GFLOPS field added in the EAR Database.
- v6.0 Added quality of life functions to management/cpupow API.
- v6.0 New API for reading metrics through Linux hwmon.
- v6.0 NVIDIA Grace Hopper devices support.
- v6.0 Added AdminUsers to provide more privileges than AuthorizedUsers.
- v6.0 Added --force option to the SLURM Plugin.
- v6.0 Added a mechanism to dynamically increase the timeout window in msg_internals when a node connects but does not return data.
- v6.0 Added stall cycles to CPI API.
- v6.0 Application CPU usage field added in the EAR Database.
- v6.0 Temperature readings through Linux hwmon.
- v6.0 Added ARM compatibility to PERF (still unstable).
- v6.0 New tool to create application signature from loop signature in DB.
- v6.0 New command enodectl.
- v6.0 Added HSMP opening mode control.
- v6.0 Added an option to ereport to query by user\_group.

### Changed
- v6.0 Renamed DBIP and DBSECIP in ear.conf to EARDBD\_IP and EARDBD\_MIRROR\_IP to better reflect what it is.
- v6.0 Energy node API.
- v6.0 EAR Loader improved.
- v6.0 Bandwidth, cache and flops APIs improved.
- v6.0 Create a fallback mechanism for failed inserts to DB.
- v6.0 Added pointer recalculation in cache\_data\_copy.

### Fixed
- v6.0 A heap-buffer overflow when opening perf files is fixed.
- v6.0 Minor fixes in the database size calculator tool.
- v6.0 Fixed erroneous time weight calculation for each loop in loops\_average.
- v6.0 Fixed SIGALRM for job validation.
- v6.0 Switch to P\_STATE 0 after userspace governor in AMD17 architecture, and a fix in PCI opened file descriptors.
- v6.0 Fixed DUMMY Cache API.
- v6.0 Fixed compilation errors when using PostgreSQL.
- v6.0 Several minor fixes in IMCFreq amd19 management API.
- v6.0 Fixed errors with ereport's -G option.

## 5.2.1 - 2025-09-02

### Added
- Added missing averaged values to eacct.
- Added averaging of power signatures.

### Changed
- Updated powercap algorithms for gpu, cpu_generic. Using LEVEL_DOMAIN in set_powercap_value calls() in powercap_mgt.c.

### Fixed
- Updated the PACKAGE_VERSION in the configure.ac file
- Add a missing ; in the application signature header csv
- Fixed serialization/deserialization of authorized users, groups and accounts.
- Fixed an error where eacct would average power consumed by multiple nodes of the application, instead of adding it.

## [5.2] - 2025-05-28

### Added
- Implementation of Powercap setting by domain.
- Application CPU utiliztion is now computed and reported by EARL to CSV files.
- Now the power signature is printed when printing an application to CSV file.
- Print all application fields in the csv file.

### Changed
- A new version of the `eacct` command is released providing an output format option.
- Reduce the periodicity in which EARL updates job's affinity mask.
- Improve roofline classification.
- `gpuprof_dcgmi` now filters unsupported events requested.

### Fixed
- Fixes related to setting powercap to single devices and domains.
- Fixed typo in cpupow define.
- Improved CPU governor list message.


## 5.1.10 - 2025-09-02

### Added
- EARGM now reads EARGMNAME variable and writes to DB and log using this name, if available.
- EARGM reads "ip:port" to support multiple eargms in the same node.
- Setting the soft powercap to unlimited when the eargm is started or restarted.
- Added reset modes (to default and to TDP) to management/cpupow.
- The ereport command can now manage list of user groups.
- Edcmon calls plugin_manager_close() when SIGINT or SIGTERM is received.

### Changed
- Using current_power instead of avg_power in dcmi node energy.
- Powermonitor is now reporting last_calculated_power instead of last_power_reported in powermon_current_power().

### Fixed
- Set the maximum CPUFreq. value when the nominal+1000 is not available on systems using intel_cpufreq driver.
- Fix powercap reset to the wrong value for AMD CPUs.
- Change setgid bits in the rpm spec file.
- Cleaned CPUPOW warnings.
- Fixed acpi_cpufreq.c segmentation fault error.
- Fixed plugin_manager_close() and plugin_manager_after_close functions.
- Fixes in gpu powercap init and logic.
- Fixed serialization and deserialization of authorized users, groups and accounts.

## 5.1.9 - 2025-04-28

### Added
- --verbose flag in the plugin manager.

### Fixed
- Avoid the usage of DRAM power in the per-core power model since it is not available in AMD systems.
- econtrol's --hosts flag output fixed.

## 5.1.8 - 2025-04-10

### Fixed
- Use ear.conf CoefficientsDir parameter instead of EAR\_ETC/coeffs as coefficients path in energy models in EARL.
- Fix bug in pmgt\_disable() disabling non-loaded domains.
- Avoid multiple master processes in NTASK\_WORKSHARING use case by increasing timeout for syncronization.

## 5.1.7 - 2025-03-27

### Changed
- Disable the inclusion of configuration files in the ear.conf.

## [5.1.6] - 2025-03-22

### Added
- Support for reading GPU power of Intel PVC devices through Linux hwmon interface added.
- CI pipeline added.
- Added carbon footprint calculations to ereport.

### Changed
- `econtrol` displays a `-` character when there is not any job running on a node for which `econtrol --status` is requested.
- EAR Data Center monitor reads the node energy plug-in from the `ear.conf` file.
- Restore GPU frequency to the maximum when a job finishes under ForceFrequencies or EARL.
- Changes in RPM spec files.
- The CHANGELOG format follows the one proposed by [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

### Fixed
- Overhead module usage fixed.
- Prevent FP exception when casting a decimal number after checking whether it is zero.
- Prevent SQL injections through `eacct` command.
- `energy_cpu_gpu` plug-in `is_null` method fixed.
- Prevent loading MySQL/Mariadb plug-ins when runnig EAR commands that load MySQL/MariaDB Connector libraries.

## EAR 5.1.5
- Bugs fixed:
  - Prevent running code in EARD report plug-in misc method if the event type is not WF_APPLICATION
  - Prevent closing a popen structure if the process is already death.
  - file_ module's function naming changed to ear_file to prevent symbol name collisions with third-party applications.

## EAR 5.1.4
- Change the open file limit in edcmon.
- Increase the timeout for remote calls.
- Don't check for socket alive to reduce remote message parsing overhead.
- Add new signature classification changes threshold for AMD zen3.
- eUFS and MPI Load Balance disabled by default.
- Bugs fixed:
  - Minor fixes in signature different decision algorithm.
  - Dimension in hsmp structure fixed.

## EAR 5.1.3
- Bugs fixed:
  - Prevent closing remote connection.
  - Increase timeout limit for remote connections.

## EAR 5.1.2
- Bugs fixed:
  - Prevent closing eard fd 0 when storing powermon\_app fd into shared memory.
  - Close folder fd when removing it.
  - GFLOPS aggregation.
  - FreeIPMI energy plug-in was managing a dynamically allocated buffer in the wrong way.
- Check whether new fds are not attended.
- Now intel\_pstate driver creates the CPUFreq. list using Boost frequency by default.

## EAR 5.1.1
- Bug fixed: The application local\_id attribute is now copied to the power monitor context when saving the application signature.
- Per-process CPU utilization fixed.
- Classify module default verbosity fixed.
- econtrol's manpage with improved examples.
- Support for GPU models in the Configuration file.

## EAR 5.1.0
- CPU temperature monitoring included in application monitoring and reported to csv files.
- Prevent workflows where all applications see all GPUs and all of them change GPU frequency.
- Support for Python multiprocess module.
- EARL is compiled by default with LITE mode. EAR Library will run in monitor mode if EARD is not present nor detected.
- Nvidia DCGM metric collection enabled by default.
- Fix DCGM application-level metrics computation.
- Prevent closing fd 0 on NTASK_WORKSHARING use case.
- Avoid collapsing application's stderr channel when EARL has high verbosity.
- Checking for authorized groups fixed. Support for multiple groups per user included.
- Added a tool for  creating application-level signatures csv file from loop signatures csv file.
- EARL Loader extension for detecting Python MPI flavour more automatically.
- Fixed an error with EARD remote connections not being properly closed.
- Add --domain option to econtrol.

## EAR 5.0.4
- Bug fixed: Missing earplug.so installation from rpm.

## EAR 5.0.3
- EARD local API creates an application directory if a third-party program connects with it.
- Fixed a typo in ereport queries.
- Prevent closing fd 0 on NTASK\_WORKSHARING use cases.
- Prevent closing fd 0 when initiating earl\_node\_mgr\_info.

## EAR 5.0
- Workflows support. Automatic detection of applications executed with same jobid/stepid.
- Fixed Intel PSTATE driver to avoid loading if there is a driver already loaded.
- Robustness improved.
- OneAPI support for Intel PVC GPUs.
- EAR Data Center Monitoring component added.
- Improved metrics and management APIs.
- Detect the interactive step on slurm systems >= 20.11 if LaunchParameters contains use\_interactive\_step in slurm.conf.
- Support for getting NVIDIA DCGM metrics.
- enode\_info tool added.
- Process resource usage is now reported by the EAR Library logs.
- Support for non-MPI multi-task applications, when tasks are spawned at invokation time, not from the application itself.
- Fixes in EAR Loader to support MPI application when MPI symbols can not be detected.
- GPU GFLOPS are now estimated and reported when using NVIDIA GPUs.

## EAR 4.3.1
- Documentation typos fixed.
- EAR configuration files templates updated.
- Bugs fixed for intel\_pstate CPUFreq driver support.
- Powercap bug fixes.
- ear.conf parsing errors found and fixed.

## EAR 4.3
- MPI stats collection now is guided by sampling to minimize the overhead.
- EARL-EARD communication optimized.
- EARL: Periodic actions optimization.
- EARL: Reduce time consumption of loop signature computation.
- erun: Provide support for multiple batch schedulers.
- eardbd: Verbosity quality improved.
- AMD Genoa is supported now.
- Improved robustness in metrics computation to support hardware failures.

## EAR 4.2
- Improved support for node sharing : save/restore configurations
- AMD(Zen3) CPUs
- Intel(r) SST support ondemand
- Improved Phases classification
- GPU idle optimization in all the application phases 
- MPI load balance for energy optimization integrated on EAR policies
- On demand COUNTDOWN support for MPI calls energy optimization
- Energy savings estimates reported to the DB (available with eacct)
- Application phases reported to the DB (available with eacct)
- MPI statistics reports: CSV file with MPI statistics
- New Intel Node Manager powercap node plugin
- Improvements in the Meta-EARGM and node powercap
- Improvements in the Soft cluster powercap
- New report plugins for non-relational DB: EXAMON, Cassandra, DCDB
- Improvements in the ear.conf parsing
- Improved metrics and management API
- Changes in the environment variables have been done for homogeneity

## EAR4.1.1
- Select replaced by poll to support bigger nodes
- Minor changes in edb_create and FP exceptions fixes

## EAR4.1
- Meta EARGM
- Support for N jobs in node
- CPU power models for N jobs
- Python apps loaded automatically
- Suport for MPI+Python through env var.
- Reports plugins in EARL, EARD and EARDBD
- Postgress support
- Soft cluster powercap
- New AMD virtual p-states support using max frequency and different P-states
- New RPC system in EARL-EARD communication (including locks)
- Partial support for different schedulers (PBS)
- New task messages between EARPlug and EARD
- New DCMI and INM-Freeipmi based energy plugins
- Likwid support for IceLake Meory bandwith computation
- Icelake support 
- msr_safe 
- HEROES plugin

## EAR4.0
- AMD virtual p-states support and DF frequency management included
- AMD optimization based on min_energy and min_time
- GPU optimization in low GPU utilization phases
- Application phases IO/MPI/Computation detection included
- Node powercap and cluster powercap implemented: Intel CPU and NVIDIA GPUS tested. Meta EAR-GM not released
- IO, Percentage of MPI and Uncore frequency reported to DB and included in eacct
- econtrol extensions for EAR health-check

## EAR3.4
- AMD monitoring support
- TAGS support included in policies
- Request dynamic in eard_rapi
- GPU monitoring support in EAR library
- Node powercap and cluster power cap under development
- papi dependency removed

## EAR3.3 vs EAR3.2
- EAR loader included
- GPU support migrated to nvml API
- TAGS supported in ear.conf
- Heterogeneous clusters specification supported
- EARGM energy capping management improved
- Internal messaging protocol improved
- Average CPU frequency and Average IMC frequency computation improved

## EAR3.2
- GPU monitoring based on nvidia-smi command
- GPU power reported to the DB
- Postgress support
- freeipmi dependence removed
