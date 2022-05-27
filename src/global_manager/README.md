

# EARGM behaviour

There are 3 main EARGM modes:
 - Cluster energy cap: Controls cluster energy consumption and ensures that it does not
 	exceed the set value. Untested with multiple EARGMs. If the total energy consumed
	approaches certain thresholds, a message is sent to the nodes to tone down the 
	application settings and/or default freq, making them use less power and energy. 
 - Cluster power cap (fine grained): Each EARGM controls the power consumption of the 
 	nodes under them and redistributes a certain budget between the nodes, allocating
	more to nodes who need it. It guarantees that any node has its default powercap
	allocation (defined by the powercap field in the tags section of ear.conf) if it 
	is running an application.
 - Cluster power cap (unlimited): Each EARGM controls the power consumption of the 
 	nodes under them by ensuring the global power does not exceed a set value. If 
	it approaches said 	value, a message is sent to all nodes to set their powercap 
	to a pre-set value (via	max_powercap in the tags section of ear.conf). Should 
	the power go back to a value under the cap, a message is sent again so the 
	nodes run at their default value (unlimited power).

# Meta-EARGM
	Meta-EARGMs can only work with fine grained cluster power cap.

# Environment variables
	There are two environment variables that can further configure EARGMS:
	- EARGMID: overrides the "node" field in EARGM configuration in ear.conf, and 
	the EARGM takes the values from the corresponding EARGMID. The node field should
	still point to that node for communication purposes.
	- EARGM_HOSTS: the EARGM will ignore their islands to control in ear.conf
	and will instead control only the nodes specified in the environment variable.
	If it is not set, the EARGM will use the islands to get the nodes.
