#!/bin/bash

# Env var starting at SLURM_ will be replaced with the Scheduler name "PBS_", "OAR_"
# Warning: select and copy those env vars needed, do not include if not needed

# Orders for EAR loader, to load the ear library with:
# Specific application name : text, no spaces.
export SLURM_EAR_LOADER_APPLICATION=
# Specific MPI version: WARNING, To be used only with MPI applications. 
# accepted values: intel, "open mpi"
export SLURM_EAR_LOAD_MPI_VERSION=

# Modifies PATHS, plugins, library version etc
# Path to your local EARL installation path. It must must be install_path/lib
export SLURM_HACK_EARL_INSTALL_PATH=
# Specific libearld.so file
export SLURM_HACK_LOADER_FILE=
# Specific libear*.so file
export SLURM_HACK_LIBRARY_FILE=
# Specific libnvidia-ml.so file
export SLURM_HACK_NVML_FILE=
# Forces the loader lo load a non-std mpi version. For example, setting to 
# "dummy" will load libear.dummy.so
export SLRURM_HACK_MPI_VERSION=

# EAR library Plugins
# Path to the .so file with the energy plugin
export SLURM_HACK_ENERGY_PLUGIN=
# Path to the .so file with the cpu energy optimization policy plugin
export SLURM_HACK_POWER_POLICY=
# Path to the .so file with the gpu energy optimization policy plugin
export SLURM_HACK_GPU_POWER_POLICY=
# Path to the .so file with the energy power model plugin
export SLURM_HACK_POWER_MODEL=
# Path to the .so file with the energy CPU power model for node sharing plugin
export SLURM_HACK_CPU_POWER_MODEL=
# List of report plugins to be added to the default ones: myplug1.so:myplug2.so
export SLURM_EAR_REPORT_ADD=

# Verbosity
# Sets the EARL verbosity level: 1..4
export SLURM_HACK_EARL_VERBOSE=
# Defines a path to generate EARL logs , 1 per node is generated
export SLURM_EARL_VERBOSE_PATH=
# Forces the EARL to generate 1 log file per process
export SLURM_HACK_PROCS_VERB_FILES=
# Enables the EAR loader verbosity: 1..4
export SLURM_EAR_LOADER_VERBOSE=
# Enables(1)/Disables(0) the generation of MPI statistics files: 
export SLURM_EAR_GET_MPI_STATS=
# Only for SLURM scheduler: Enables the verbosity of the SLURM plugin. Applies also to erun: 1..4
export SLURM_COMP_VERBOSE=

# Policy options
# Defines a default CPU frequency (in KHz)
export SLURM_HACK_DEF_FREQ=
# Defines a default GPU frequency (in KHz)
export SLURM_EAR_GPU_DEF_FREQ=
# Defines a maximum memory frequency (in KHz). Data Fabric in AMD.
export SLURM_EAR_MAX_IMCFREQ=
# Defines a minimum memory frequency (in KHz). Data Fabric in AMD.
export SLURM_EAR_MIN_IMCFREQ=
# Sets maximum and minimum memory frequency to same value (in KHz). Data Fabric in AMD.
export SLURM_EAR_SET_IMCFREQ=
# Defines the maximum extra threshold used by energy policies when reducing the memory frequency: 0..1
export SLURM_EAR_POLICY_IMC_TH=
# Enables(1)/Disables(0) the energy policies to allow the hardware to 
# select the default memory frequency. Applies to Intel and Min_time_to_solution: 1
export SLURM_EAR_LET_HW_IMC=
# Specified the job is using the job in exclusive mode. Some optimizations are applied to idle CPUs. 1
export SLURM_EAR_JOB_EXCLUSIVE_MODE=
# Enables(1)/Disables(0) the utilization of EARL phases classification
export SLURM_USE_EARL_PHASES=
# Enables(1)/Disables(0) the load balance strategy in energy policies
export SLURM_EAR_LOAD_BALANCE=
# Sets the threashold used to classify an application as unbalanced. The higher 
# the threshols the higher the difference to classy as unbalanced: Values 0..100
export SLURM_EAR_LOAD_BALANCE_TH=
# Enables(1)/Disables(0) the utilization of boost CPU frequency for processes in 
# the critical path. Applies only to unbalanced scenarios.
export SLURM_USE_TURBO_FOR_CP=
# Hin(1/0)t about the network and uncore frequency relationship to force 
# the energy policy to be more conservative in terms of memory frequency
export SLURM_EAR_NTWRK_IMC=
# Enables(1)/Disables(0) the utilization of boost CPU frequency when 
# CPU frequency selected is nominal and application is cpu bound classified
export SLURM_EAR_TRY_TURBO=
# Enables(1)/Disables(0) the utilization of energy models in energy polciies 
export SLURM_EAR_USE_ENERGY_MODELS=


# Tracing
# Specifies the path for traces. This is different from report traces
export SLURM_EAR_TRACE_PATH=
# Trace plugin to be used (.so file)
export SLURM_EAR_TRACE_PLUGIN=


