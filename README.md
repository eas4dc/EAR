# Energy Aware Runtime version 4.0

<img src="etc/images/logo.png" align="right" width="440">
Energy Aware Runtime (EAR) package provides an energy management framework for super computers. EAR contains different components, all together provide three main services:

1) A **easy-to-use and lightweight optimizarion service** to automatically select the optimal CPU frequency according to the application and the node characteristics. This service is provided by two components: the EAR library (**EARL**) and the EAR daemon (**EARD**). EARL is a smart component which is loaded next to the application, intercepting  MPI calls and selecting the CPU frequency based on the application behaviour on the fly. The library is loaded automatically through the EAR SLURM plugin (**EARPLUG, earplug.so**).


2) A complete **energy and performance accounting and monitoring system** based on SQL database (MariaDB and PostgreSQL are supported). The energy accounting system is configurable in terms of application details and update frequency. The EAR database daemon (**EARDBD**) is used to cache those metrics prior to DB insertions.

3) A **global energy management** to monitor and control the energy consumed in the system through the EAR global manager daemon (**EARGMD**). This control is configurable, it can dynamically adapt  policy settings based on global energy limits or just offer global cluster monitoring.


License
-------
EAR is a open source software and it is licensed under both the BSD-3 license for individual/non-commercial
use and EPL-1.0 license for commercial use. Full text of both licenses can be
found in COPYING.BSD and COPYING.EPL files.

Contact: [ear-support@bsc.es](mailto:ear-support@bsc.es)

