#!/bin/bash

####################### Configuration starts here #######################


# Specify source path
SRCPATH=$PWD

# EAR library versions: enabled or disabled
cray="enabled"
nompi="enabled"


# If your system doesn't support avx512 (i.e Rome AMD) enable the --disable-avx512 flag

# If gpus are disabled, remove --with-cuda flag from configure
export extra_disable="--disable-avx512"
# End disable section


# If some of these paths must be explicityly added, adapt the deploy file to load the module and add the option.
#######  End Extra configuration paths

# Installation paths
export EAR_VERSION=5.1
# tag is a text to be included as prefix for the installation path
export tag="cscs"
# Installation prefix path
export INSTALL_ROOT=$SCRATCH/EAR
export INS_PATH=$INSTALL_ROOT/ear$EAR_VERSION.$tag

export ETC=$INS_PATH/etc
export ETMP=/tmp
export DOCPATH=$INSTALL_ROOT/doc

## If some specific paths must be defined becauset there is no module, add here
export SLURMPATH=/usr

# Uncoment to install with specific users
#export US=ear
#export GR=ear

####################### Configuration
	echo "  Features enabled: " $EAR_FEATURES
  echo "  libraries compiled IntelMPI=$intelmpi OpenMPI=$openmpi NO-MPI=$nompi"
  echo "  Install path $INS_PATH"
  echo "  Sources      $SRCPATH"
  echo "  This script executes the configure, make, make install for the enabled versions. ETC is also installed with template files"
	echo "  No services are started"

if [ "$1" == "help" ]; then
	echo "Usage: ../deploy.sh install/noinstall [full]"
	echo " 	install executes configure, compiles and install"
	echo " 	noinstall executes configure"
	echo "  full executes a make install, otherwise only modified files are recompiled"
	echo "  invoked in the SRC folder"
  exit
fi

# Ear deploy script

# Modify with specific modules to be loaded
core_modules=""
intel_modules=""
nvidia_modules="cudatoolkit/21.9_11.0"
openmpi_modules=""



####################### Configuration ends here #######################

# Start preparing environment

echo "******************************** EAR installation starts here ********************************"
module swap PrgEnv-cray PrgEnv-gnu
module load  $nvidia_modules
module load cray-mpich
export CUDA_ROOT=$CUDA_HOME

export system_type="Cray_HPE"

# PATH configuration
cd $SRCPATH

echo "***** Autoreconfig ****"
autoreconf -i

### Compile warning: This script assumes the compilation node is architectural compatible with the compute node. If not, some
### modifications are needed, for example enabling some instructions or disabling it. For example, compiling in Rome node to
### support Intel AVX512 requires extra cflags options
### extra_cflags="-msse4.1  -msse4.2 -msse3 -mavx512dq -mavx512f -mavx"



if [ "$nompi" == "enabled" ]; then

echo "------------------------"
echo "Configuring NO MPI"
echo "------------------------"

#With icx
#my_cflags='-Wno-unused-command-line-argument -static-intel -Wall -Rno-debug-disables-optimization -Wno-empty-body'
#my_cflags='-Wno-unused-command-line-argument -static-intel -Wall -g -Rno-debug-disables-optimization -Wno-empty-body '

# With gcc
my_cflags='-march=native -Wall'

echo "Using CFLAGS="$my_cflags
./configure --prefix=$INS_PATH EAR_TMP=$ETMP EAR_ETC=$ETC CC=gcc CC_FLAGS="$my_cflags" MPICC_FLAGS="$my_cflags" MPICC=cc MAKE_NAME=nompi.$tag  USER=$US GROUP=$GR --docdir=$DOCPATH --disable-mpi --with-cuda=$CUDA_HOME $extra_disable --with-slurm=$SLURMPATH


if [ "$1" == "install" ]; then
echo "------------------------"
echo " Compiling NO MPI"
echo "------------------------"

eval $EAR_FEATURES make -f Makefile.nompi.$tag $2
eval $EAR_FEATURES make -f Makefile.nompi.$tag install

fi
fi



#### CRAY-MPICH
if [ "$cray" == "enabled" ]; then

echo "------------------------"
echo "Configuring CRAY-MPICH"
echo "------------------------"

my_cflags='-march=native -Wall'
echo "Using CFLAGS="$my_cflags
./configure --prefix=$INS_PATH EAR_TMP=$ETMP EAR_ETC=$ETC CC=gcc CC_FLAGS="$my_cflags" MPICC_FLAGS="$my_cflags" MPICC=cc MPI_VERSION=cray MAKE_NAME=cray.$tag  USER=$US GROUP=$GR --docdir=$DOCPATH --with-cuda=$CUDA_HOME $extra_disable


if [ "$1" == "install" ]; then
echo "------------------------"
echo " Compiling CRAY"
echo "------------------------"

eval $EAR_FEATURES make -f Makefile.cray.$tag $2
eval $EAR_FEATURES make -f Makefile.cray.$tag earl.install

fi
fi



echo "******************************** EAR installation ends here ********************************"
