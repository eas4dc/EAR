### EAR4.0
- AMD virtual p-states support and DF frequency management included
- AMD optimization based on min_energy and min_time
- GPU optimization in low GPU utilization phases
- Application phases IO/MPI/Computation detection included
- Node powercap and cluster powercap implemented: Intel CPU and NVIDIA GPUS tested. Meta EAR-GM not released
- IO, Percentage of MPI and Uncore frequency reported to DB and included in eacct
- econtrol extensions for EAR health-check
### EAR3.4
- AMD monitoring support
- TAGS support included in policies
- Request dynamic in eard_rapi
- GPU monitoring support in EAR library
- Node powercap and cluster power cap under development
- papi dependency removed

### EAR3.3 vs EAR3.2
- EAR loader included
- GPU support migrated to nvml API
- TAGS supported in ear.conf
- Heterogeneous clusters specification supported
- EARGM energy capping management improved
- Internal messaging protocol improved
- Average CPU frequency and Average IMC frequency computation improved

### EAR3.2
- GPU monitoring based on nvidia-smi command
- GPU power reported to the DB
- Postgress support
- freeipmi dependence removed
