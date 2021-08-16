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
		

