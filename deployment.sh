#!/bin/bash
  export EAR_INSTALL=$HOME/ear4.2_lenox
  export EAR_INSTALL=/hpc/base/ctt/packages/EAR/ear4.2
  export EAR_TMP=/var/ear
  export EAR_ETC=$EAR_INSTALL/etc
  module purge
  module load base-env
  module load compiler/gnu/11.2.0
  module load cuda/11.0
  module load likwid/5.2.1
  module load mpi/intel/2021.2
  module load compiler/intel/20.4
  module load cupti/10.1

  autoreconf -i

  echo "Intel MPI"
  ./configure --prefix=$EAR_INSTALL EAR_TMP=/var/ear EAR_ETC=$EAR_INSTALL/etc --with-slurm=/hpc/base/ctt/build/slurm-21.08.4 --with-likwid=$LIKWIDROOT --with-cuda=$CUDA_HOME CC=icc MPICC=mpiicc CC_FLAGS="-static-intel -Werror-all -g" MPICC_FLAGS="-static-intel -Werror-all -g" --with-cupti=$CUDA_HOME/extras/CUPTI --with-oneapi=$HOME/level_zero MAKE_NAME=icc.impi.cupti.oneapi
  echo "SLURM_CFLAGS     = -I/hpc/base/ctt/build/slurm-21.08.4" >> Makefile.icc.impi.cupti.oneapi

  module unload mpi/intel/2021.2
  module unload compiler/intel/20.4
  module load mpi/openmpi/4.1.3_gnu_cuda

  echo "OpenMPI"
  ./configure --prefix=$EAR_INSTALL EAR_TMP=/var/ear EAR_ETC=$EAR_INSTALL/etc --with-slurm=/hpc/base/ctt/build/slurm-21.08.4 --with-likwid=$LIKWIDROOT --with-cuda=$CUDA_HOME CC=gcc MPICC=mpicc CC_FLAGS="-Wall -g" MPICC_FLAGS="-Wall -g" --with-cupti=$CUDA_HOME/extras/CUPTI --with-oneapi=$HOME/level_zero MAKE_NAME=gcc.ompi.cupti.oneapi MPI_VERSION=ompi
  echo "SLURM_CFLAGS     = -I/hpc/base/ctt/build/slurm-21.08.4" >> Makefile.gcc.ompi.cupti.oneapi

echo "No MPI"
  ./configure --prefix=$EAR_INSTALL EAR_TMP=/var/ear EAR_ETC=$EAR_INSTALL/etc --with-slurm=/hpc/base/ctt/build/slurm-21.08.4 --with-likwid=$LIKWIDROOT --with-cuda=$CUDA_HOME CC=gcc MPICC=mpicc CC_FLAGS="-Wall -g" MPICC_FLAGS="-Wall -g" --with-cupti=$CUDA_HOME/extras/CUPTI --with-oneapi=$HOME/level_zero MAKE_NAME=gcc.nompi.cupti.oneapi --disable-mpi
  echo "SLURM_CFLAGS     = -I/hpc/base/ctt/build/slurm-21.08.4" >> Makefile.gcc.nompi.cupti.oneapi

