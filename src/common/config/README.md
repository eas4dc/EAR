# System configuration default values

For each system it is recommended to create a specific config\_def.h file. For new flags add them in configi\_def\_HPC.h file (default values)

## Energy optimization options
> - USE\_TURBO : Uses the turbo frequency by default in energy optimization policies
> - RESET\_FREQ : forces the energy optimization policies to go to default frequencies before re-appling the policies. Do that when energy models accuracy depends on freuency.
> - EAR\_eUFS : Enables/Disables the utilization of dynamic memory management by default for the energy optimization policies. 
> - EAR\_USE\_BUSY\_WAITING : Enables/Disables the dynamic classification of CPU busy waiting by the energy optimization policies. Minimizes the possibility to select very low CPu freqs.

## Other configuration options

> - DEF\_EARL\_LOOP\_REPORTED: Enable/Disable by default the loop signature report to DB for all applications using EARL
> - USE\_IDLE\_PC\_MINFREQ : Allows the node powercap to assume idle nodes when starting the EARD and then using min CPU frequency when idle. If EARD is started (or can be) with the node in production it must be set to 0.
> - EARGM\_POWERCAP\_SOFTPC\_HOMOGENEOUS : Configures the cluster soft powercap policy as to distribute the power equally between all the compute node. Recommended for homogeneous architectures.
> - CPUFREQ\_SET\_ALL\_USERS : Enable/Disable the capability to ask for specific CPU frequencies for all users ignoring the Authorized permission
> - DYNAMIC\_BOOST\_MGT : Applies only to intel\_pstate configurations. Forces the apis to create the list of frequencies assuming turbo is enabled. Allows dynamic on/off of turbo by scheduler flags.
> - RUN\_AS\_ROOT : Forces the EAR services to be executed as root for security reasons.
> - FORCE\_EAR\_LOG\_FILES\_AS\_ROOT  : Forces system log files to be created without OTHER permissions. Only root can read them
