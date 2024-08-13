# EAR states

When EAR is driven by DynAIS, it applies a state diagram. The following states are now considered (see src/library/states.c or updates). Main states are in bold. Other states are mostly transition states.

* No loop detected
	* NO_PERIOD: This is the initial state and stands for no iteration detected. Once we enter 				
* Loop detected
	* FIRST_ITERATION: This state starts computing the signature			
	* **EVALUATING_LOCAL_SIGNATURE** : This state computes the local signature and executes the per-node policy. Depending on the policy readiness, the new state will be the following one:
		* If EAR_POLICY_READY && Frequency changed --> new_state = RECOMPUTING_N
		* If EAR_POLICY_READY && !Frequency changed --> new_state = SIGNATURE_STABLE
		* If EAR_POLICY_CONTINUE --> new_state = EVALUATING_LOCAL_SIGNATURE
		* If EAR_POLICY_GLOBAL_EV --> new_state = EVALUATING_GLOBAL_SIGNATURE	
	* RECOMPUTING_N: This is a transition state to recompute the N just in case we need to adapt it because of the frequency selection. new_state = SIGNATURE_STABLE
	* **SIGNATURE_STABLE** : This state computes a new signature and evaluates the accuracy of the policy using the policy_ok function. Depending on the evaluation, three new states are possible
		* SIGNATURE_STABLE: If policy is ok, we stay in this state. This is the ideal case.
		* PROJECTION_ERROR: If policy is not ok and we try to select the good frequency up to a maximum number of tries, we asume there is a problem with the model accuracy and we stop the frequency selection with the default policy. This is a final state and the worst scenario.
		* SIGNATURE_HAS_CHANGED: If the policy is not ok beacuse the signature has changed, we re-compute the signature.		
	* PROJECTION_ERROR : new_freq= default freq. new_state=PROJECTION_ERROR
	* SIGNATURE_HAS_CHANGED	: Same than RECOMPUTING_N.
	* TEST_LOOP:This state is considered for optimization. It prevents EAR to start computing the signature at application start when many small loops are detected. new_state = FIRST_ITERATION		
	* **EVALUATING_GLOBAL_SIGNATURE** : This state applies the per-application policy. The policy will internally check if global metrics are ready. Depending on the policy readiness, the new state will be the following one:
		* If EAR_POLICY_READY && Frequency changed --> new_state = RECOMPUTING_N
		* If EAR_POLICY_READY && !Frequency changed --> new_state = SIGNATURE_STABLE
		* If EAR_POLICY_CONTINUE --> new_state = EVALUATING_GLOBAL_SIGNATURE

[Here](https://nextcloud.bsc.es/index.php/s/Hcs2dLTPFwFmAaT) is the state diagram of the **states** module.
It might be outdated, but records the most important overview of states transitions.


## Signatures used

### tracer_signature

Used just to report per-process signature on some events.
It is filled from each process' shared signature by calling `signature_from_ssig`.
Then, the attribute `def_freq` is filled by the process shared signature's `new_freq` attribute.
Finally, it is an argument of `traces_new_signature`.

The above procedure is called on `states_new_iteration`:
- Every five iterations.
- At the end of **EVALUATING_LOCAL_SIGNATURE** and **SIGNATURE_STABLE**, just before verbosing the loop signature.

### policy_sel_signature

It is just a copy of the node `job_signature` (**EVALUATING_LOCAL_SIGNATURE**).
As the reference signature of the previous iteration, compared against `loop_signature` to see if they differ more than a 20% (**SIGNATURE_STABLE**).
On the same state, passed as the reference signature on the `policy_ok` method.
Finally, it is passed to the method `policy_had_effect`.

### curr_loop

It is a `loop_t` because loop is the type of the reporting system, but its most important attribute it the loop signature.
At the begining of each period, it is cleaned (all allocated bytes set to 0), and it is passed to policy `loop_init` method.

At **EVALUATING_LOCAL_SIGNATURE**:
- After computing the loop signature (stored at `job_signature`), the **master** computes the job node signature and stores it on `curr_loop`'s signature.
- **Slaves can't use this signature's extended signature**.
- Finally, it is reported.

At **SIGNATURE_STABLE**:
- Passed as argument to `compute_per_process_and_job_metrics`. Result of the CPU power model (node level) is stored on `job_signature`, then copied to `curr_loop` signature.
- Then, the same procedure as the above is applied.

### loop_signature

- Its extended signature is allocated at the beginning of each job.
- It is the most important signature of each loop, where loop signature is computed.
- Early filled with GFLOP/s, I/O and GPU data on each new iteration.

At **EVALUATING_LOCAL_SIGNATURE**:
- The loop signature computed by `metrics_compute_signature_finish` is stored here.
- Phases accumulated to the global application signature, so the extended signature is being used and maintained.
- The selected memory frequency is computed and store here.

At **SIGNATURE_STABLE**:
- The loop signature computed by `metrics_compute_signature_finish` is stored here.
- After computing the node `job_signature`, contents of `curr_loop` are copied to `loop_signature`.
- Used as *current signature* argument of `signatures_different` method.
- Energy savings accumulated to global application signature from this signature.
- Used as *current signature* argument of `policy_ok` method call.

### application.signature

This is the global application signature.
Phases summary and energy savings are accumulated here.
From the point of view of states module, this signature is not used anywhere.

### lib_shared_region->job_signature

- It is the reference (or *master*) signature when calling `metrics_job_signature`. The accumulated node job signature computed is stored then here.
- It is the reference signature when calling the `adapt_signature_to_node` method.
- This signature is updated at every signature computation along with loop\_signature but internally on library/metrics.
- **Question** Which is the difference between this signature and `lib_shared_region->node_signature`?

### norm_signature

It is taken from `lib_shared_region->job_signature` to have a signature ready to fit energy models.
**ATENTION** The method `adapt_signature_to_node` is just a signature copy, so we can remove this signature.
