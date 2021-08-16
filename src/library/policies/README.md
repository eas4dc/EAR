
# Policies

## List of policies
The list of official supported policies are:

- monitoring: This policy is not a real policy, it only offers application performance and power profiling
- min_time: This policy starts at a default frequency lower than nominal and increases the frequency up to the nominal in case the application presents a good performance scalability. It uses the CPU energy models
- min_energy: This policy starts at nominal frequency and reduces the frequency to minimize the energy with a limit in the performance degradation. It uses the CPU energy models



## Additional files to support dynamic policy setting management
- When using EARGM, policy settings can be dynamically modified to adapt the system to the energy limits. Files with extension \_eard are loaded by the eard and implements the functions to react to EARGM requests

## Policy selection
	- the CPU policy is selected using the --ear-policy flag when submitting a job. The GPU policy is selected based on the CPU policy

## Policy API

EAR supports energy policies as plugins. The policy API is defined in policy.c. Till version 3.3 only local (per-node) policies were supported. Version 3.4 includes per-application policies. Policy functions are optional and the generic policy API checks each function before calling it. The list of functions is (check policy.c for updates)

	- policy_init
	- policy_apply
	- policy_app_apply
	- policy_get_default_freq
	- policy_ok
	- policy_max_tries
	- policy_end
	- policy_loop_init
	- policy_loop_end
	- policy_new_iteration
	- policy_mpi_init
	- policy_mpi_end
	- policy_configure

Same API is used for CPU and GPU.


*policy_apply* refers to per-node policy selection and *policy_app_apply* refers to per-application frequency selection. 

## Policy states

Depending on the readiness of the policy, the EAR sates transitions changes. 

	- EAR_POLICY_READY : The policy has finished, a new frequency has been selected and the new state must be applied. Refers to per-node policies
	- EAR_POLICY_CONTINUE: The policy needs more time in the same state. EAR state is not modified. Refers to per-node and per-application policies.
	- EAR_POLICY_GLOBAL_EV: The policy uses a per-application policy and a global signature evaluation or per-application state must be applied
	- EAR_POLICY_GLOBAL_READY: The per-application policy is ready and the new state must be applied. 

## Local (per-node) signatures
Per-node signatures are computed by src/library/metric.c file. A per-node signature computation is started automatically after the last one is computed. It should be re-started manually in case the frequency is modified after it was started to guarantee it is computed with a constant frequency.

    




