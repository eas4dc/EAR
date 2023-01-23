#!/bin/bash
#SBATCH --ntasks-per-node=12
#SBATCH -N 2
#SBATCH -n 24

export EAR_INSTALL_PATH=/hpc/opt/ear
export EAR_ETC=/hpc/opt/ear/etc
export EAR_TMP=/var/run/ear
export EAR_VERBOSE=1
export LD_PRELOAD=${EAR_INSTALL_PATH}/lib/libearld.so
srun --task-prolog=${EAR_INSTALL_PATH}/bin/newjob.sh --task-epilog=${EAR_INSTALL_PATH}/bin/endjob.sh  --export=ALL  -n 24 ./bt-mz.C.24

#mpirun -n 24 erun --program=./bt.C.24

