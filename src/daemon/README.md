# Job submission data: Shared regions between EARD-EARL

### Powermonitor contexts
- Each new job_id,step_id created a new "context" in the powermonitor thread. EARD assumes there are N nested contexts but only 1 is active (the last one) and it's the one reported by the power  monitoring thread
- These contexts are managed like a vector of powermon_apps defined in power_monitor.h

### Shared memory regions
- Shared memory regions shared between EARD and EARL
	- Coefficients: $EAR_TMP/.ear_coeffs. Coefficients are read only data, they can be shared between all the jobs in a node. There is also a $EAR_TMP/.ear_coeffs_default file with default coefficients.
	- Application settings: $EAR_TMP/ear_settings_conf. There is a single area assuming there is only one job/step active. This region includes application settings such as frequency, policy, etc. It's read only for the application. 
	- Rescheduling: $EAR_TMP/.ear_resched. This is a small region (int) acting like a boolean. It means there has been some change in policy settings or in general app configuration and application must reapply the policy. This is a RD/WR region.
	- Application information: $EAR_TM/.ear_app_mgt, this region includes application information per application. It's a RD/RW region. This information includes the number of processes, the number of ppn, the number of nodes etc.
	- Powercap information: $EAR_TMP/.ear_pc_app_info. This region includes powercap related information. It's a RD/WR region. Includes information such as the powercap and the requested frequncies.



## Additional job submission info used by SLURM plugin
- Services. $EAR_TMP/.ear_services_conf. The list of services information. This is a read only region
- Frequencies: $EAR_TMP/ear_frequencies. The list of frequencies. This is a read only region. This shared area is not dynamically updated, only at the EARD init.


