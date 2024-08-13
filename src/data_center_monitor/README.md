# List of EDCMON plugins

- conf.so : tag="conf". Reads the EAR configuration   file (ear.conf). EAR\_ETC has  to be used. It's an auxiliary plugin to be used by the other pplugins. No dependencies. 
- dummy.so : tag="dummy". API template. No requiremens. Depends on "conf" 
- eardcon.so : tag="eardcon". Connects with the eard. To be used when no root privileges are available and needed. No dependencies. Requirement: To be executed in the same node than EARD.
- keyboard.so : tag="keyboard". No dependencies. Description missing.
- management.so : tag="management". Depends on "conf" and "metrics". Initializes the EAR management library. 
- metrics.so : tag="metrics". Depends on "conf". Initializes the EAR metrics library and read all the metrics periodically.
- periodic\_metrics.so : tag="periodic\_metrics". Depends on conf and metrics. Loads same report plugins than the eardbd. Periodically creates a periodic\_metric data and reports it top the DB. Implements the post method to update the jobid/stepid of the periodic\_metric info.

