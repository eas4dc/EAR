## Unreleased
- Removed documentation files
- Fixed when Nvidia-SMI returns bad strings.
- Added ERUN, a component to simulate the SLURM Plugin process when SLURM is not present.
- Frequency control included in eard
- eard control for node non existing in configuration
- GPU support for power monitoring. Not all cases supported yet
- Extensions to report GPU in DB
- Postgress support
- Automatic GBs check in EARL

## Changed
- cluster_conf_read error fixed when reading "privileged" specification for policies
- ear.conf and ear.conf.full replaced by ear.conf.template and ear.conf.full.template
- More info with SLURM_COMP_VERBOSE env var
- ecct modififed to remove MAX_SIG_POWER and MIN_SIG_POWER
- error fixed when using short ear.conf files
- rpms folder modified to support rellocatable rpms /usr and /etc paths 
- energy_nm had an error when energy_init = energy_end
- bandwith.c modified to include NULL pointers when architecture is not detected. It is pending to migrate it to a plugin
- energy_nm updated to be the same installed in lennox
- metrics folder modified by topics, msr common for rapl and temperature included in msr
- Added '--disable-avx512' flag to configure to use AVX2 symbols instead of AVX512. It is required when working with Haswell/Broadwell systems or older. Also added '--with-fortran' flag to configure to add Fortran symbols to the EAR library. It is required when working with some MPI distributions such as OpenMPI. Finally, configure accepts PostgreSQL flags.
- Three new functions in ear_api for manual utilization of EAR (requires application modification)
- Option in EARPlug to specify trace pathname changed to SLURM_EAR_TRACE_PATH.
- new trace plugin mechamism. EAR_GUI is set to on by default. SLURM_EAR_TRACE_PLUGIN env var defines trace path (no default location). Paraver plugin uses SLURM_EAR_TRACE_PATH env var. Pending to check the plugin.
- EARD INIT and RT errors reported as events to the DB: Not fully tested
- rpms3 folder included for rpm testing
- EARplug including SLURM_HACK_LIBRARY for local EARL utilization
- Support for multiple LD_PRELOAD libraries in EARplug
- Default plugins included in EAR
- EAR cpupower included 
- Merge with new_policies branch: rapl plugin for energy supported
- Added SLURM_EAR_MPI_VERSION to automatic selection of ear library  in EARplug
- IPMI finder removed from energy loading. Default is not supported
- power_cap and power_cap_type included in island for power_capping policies
- New policies and power models included for testing
- Merged with new policies in ear.conf
- Power policies loaded with plugins 
- Power models loaded with plugins (first prototype). dir_plug included to be used with plugins
- The energy reading system now works through plugins. (5bf9dddcbe0d815cc9bd9a39ccc296cf1c29bfb9)
- eargm,earlib,daemon warnings fixed with -Wall and gcc 8
- verbose messages converted to debug or error
- error.h and debug.h included in verbose.h
- minor change fixed in wait_for_cient function. Argument for accept was incorrect.
- eard_api non-blocking calls
- increased  MAX_TRIES for non-blocking calls
- NO_COMMAND set when message failure and non-blocking calls are used
- eargmd. Warning was not initialized for NO_PROBLEM case
- Working in a dynamic loading for power/energy policies
- Cleaned COORDINATE_FREQUENCIES, MEASURE_DYNAIS_OV, EAR_PERFORMANCE_TESTS and IN_MPI_TIME.
- Cleaned dynamic policies.
- Deleted unused files.

### Changed
- new IPMI interface thread-safe. Each EARD thread creates a new energy_handler_t
- lock to avoid simultaneous ipmi access
- assert removed from ipmi functions. replaced by condition+error message
- eard_rapi new_job and end_job calls are now non-blocking
- support for dynamic management of multiple contexts

- earl: error.h included in ear_api.c. It was generating a crash when invalid step_id is used
- earl: master lock files removed when invalid step id is used

- eardbd_api error when using global variables. eardbd_api is not thread-safe. Two variables moved from global to local and loc    k included in sockets_send for atomic sent
- eardbd host name included in aggregated metrics

- eargm reporting gloabl status every T1 period

- new ereport with options -i (filter by island) and -g (show global manager records)
- ereport now has a filter for eadbds/islands
- ereport now has an option to report Global_energy records
- ereport -i "all" option now also reports avg power
- ereport -g now can be used in conjunction with -s
- new eacct -x option to see EAR events
- eacct does not filter applications with high and low power values anymore

- fixed an error where edb_create would not output a correct user creation query
