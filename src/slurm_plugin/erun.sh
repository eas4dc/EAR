#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --tasks-per-node=4
#SBATCH --nodelist=cmp2545

echo EAR_INSTALL_PATH $EAR_INSTALL_PATH
echo EAR_TMP $EAR_TMP
echo EAR_ETC $EAR_ETC
echo EAR_DEFAULT $EAR_DEFAULT
echo SLURM_ERSBAC $SLURM_ERSBAC
echo SLURM_ERSRUN $SLURM_ERSRUN
export EAR_DEFAULT="on"

#export PATH=$PATH:/hpc/opt/intel/compilers_and_libraries_2017.7.259/linux/bin/intel64
export PATH=$PATH:/hpc/opt/intel/compilers_and_libraries_2017.7.259/linux/mpi/intel64/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/hpc/opt/intel/compilers_and_libraries_2017.7.259/linux/mpi/intel64/lib/
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/hpc/opt/intel/compilers_and_libraries_2017.7.259/linux/compiler/lib/intel64_lin/
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/hpc/opt/intel/compilers_and_libraries_2017.7.259/linux/mkl/lib/intel64_lin/

#unset EAR_ETC
SLURM_COMP_VERBOSE=4 mpirun ./erun --program=hostname --ear-verbose=1 --ear-policy=min_energy
#SLURM_COMP_VERBOSE=2 mpirun ./erun --program=env --ear-verbose=1 --ear-policy=min_time
#SLURM_COMP_VERBOSE=2 srun --mpi=pmi2 ./erun --program=/hpc/base/ctt/packages/EAR/1908/bin/kernels/bt-mz.C.40 --ear-verbose=1
#SLURM_COMP_VERBOSE=2 srun --mpi=pmi2 ./erun --program=/hpc/base/ctt/packages/EAR/1908/bin/kernels/sp-mz.C.40 --ear-verbose=1

