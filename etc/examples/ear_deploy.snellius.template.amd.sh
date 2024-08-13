#!/bin/bash

####################### Configuration starts here ####################### 

## Add here the external features you want to enable in all the cases. 

# Enable DCGMI metrics and allows ear-lite support
#EAR_FEATURES="DCGMI=1 FEAT_EARL_LITE=1"

# Enable DCGMI metrics
EAR_FEATURES="DCGMI=1"

# No extra features
#EAR_FEATURES=""

# Specify source path
SRCPATH=$PWD

# EAR library versions: enabled or disabled
openmpi="enabled"
nompi="enabled"
intelmpi="enabled"

# Start disable section:  Empty by default
  #--enable-arm64          Compiles for ARM64 architecture
  #--disable-avx512        Compiles replacing AVX-512 instructions by AVX-2
  #--disable-gpus          Does not allocate GPU types nor report any

# If your system doesn't support avx512 (i.e Rome AMD) enable the --disable-avx512 flag

# If gpus are disabled, remove --with-cuda flag from configure
export extra_disable="--disable-avx512"
# End disable section

#### Extra configuration paths section. Not included by default

  #--with-rsmi=PATH        Specify path to AMD ROCM-SMI installation
  #--with-oneapi=PATH      Specify path to OneAPI Level Zero installation
  #--with-gsl=PATH         Specify path to GSL installation
  #--with-mysql=PATH       Specify path to MySQL installation
  #--with-slurm=PATH       Specify path to SLURM installation
  #--with-oar              Specify to enable OAR components
  #--with-pbs              Specify to enable PBS components
  #--with-dlb=<PATH>       Specify a valid DLB root path to provide support for
                          #colleting MPI node metrics by using DLB TALP API
                          #(disabled by default).
  #--with-countdown=<PATH> Specify a valid Countdown root path to provide
                          #support for loading the Countdown library (disabled
                          #by default).

# If some of these paths must be explicityly added, adapt the deploy file to load the module and add the option.
#######  End Extra configuration paths

# Installation paths
export EAR_VERSION=5.0
# tag is a text to be included as prefix for the installation path
export tag="amd.wf"
# Installation prefix path
export INSTALL_ROOT=$HOME/EAR
export INS_PATH=$INSTALL_ROOT/ear$EAR_VERSION.$tag

export ETC=$INS_PATH/etc
export ETMP=/var/ear
export DOCPATH=$INSTALL_ROOT/doc

## If some specific paths must be defined becauset there is no module, add here

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

core_modules="2023"
intel_modules="iimpi/2023a"
nvidia_modules="CUDA/12.1.1"
openmpi_modules="gompi/2023a"


####################### Configuration ends here ####################### 

# Start preparing environment

echo "******************************** EAR installation starts here ********************************"
module purge
module load $core_modules

# PATH configuration
cd $SRCPATH

echo "***** Autoreconfig ****"
autoreconf -i

### Compile warning: This script assumes the compilation node is architectural compatible with the compute node. If not, some
### modifications are needed, for example enabling some instructions or disabling it. For example, compiling in Rome node to
### support Intel AVX512 requires extra cflags options 
### extra_cflags="-msse4.1  -msse4.2 -msse3 -mavx512dq -mavx512f -mavx"

if [ "$intelmpi" == "enabled" ]; then
module load  $intel_modules
module load  $nvidia_modules

echo "------------------------"
echo "Configuring IntelMPI"
echo "------------------------"

#my_cflags='-Wno-unused-command-line-argument -static-intel -Wall'
my_cflags='-Wno-unused-command-line-argument -static-intel -Wall -g -Rno-debug-disables-optimization -Wno-empty-body'
echo "Using CFLAGS="$my_cflags

./configure --prefix=$INS_PATH EAR_TMP=$ETMP EAR_ETC=$ETC CC=icx CC_FLAGS="$my_cflags" MPICC_FLAGS="$my_cflags" MPICC=mpiicc MAKE_NAME=intelmpi.$tag   USER=$US GROUP=$GR --docdir=$DOCPATH --with-cuda=$CUDA_ROOT $extra_disable

if [ "$1" == "install" ]; then
echo "------------------------"
echo " Compiling IntelMPI"
echo "------------------------"

eval $EAR_FEATURES make -f Makefile.intelmpi.$tag $2
eval $EAR_FEATURES make -f Makefile.intelmpi.$tag install
eval $EAR_FEATURES make -f Makefile.intelmpi.$tag etc.install
eval $EAR_FEATURES make -f Makefile.intelmpi.$tag doc.install

fi
module unload $nvidia_modules
module unload $intel_modules
fi

if [ "$nompi" == "enabled" ]; then
module load  $openmpi_modules
module load  $nvidia_modules

echo "------------------------"
echo "Configuring NO MPI"
echo "------------------------"

#With icx
#my_cflags='-Wno-unused-command-line-argument -static-intel -Wall -Rno-debug-disables-optimization -Wno-empty-body'
#my_cflags='-Wno-unused-command-line-argument -static-intel -Wall -g -Rno-debug-disables-optimization -Wno-empty-body '

# With gcc
my_cflags='-march=native -Wall'

echo "Using CFLAGS="$my_cflags
./configure --prefix=$INS_PATH EAR_TMP=$ETMP EAR_ETC=$ETC CC=gcc CC_FLAGS="$my_cflags" MPICC_FLAGS="$my_cflags" MPICC=mpicc MAKE_NAME=nompi.$tag  USER=$US GROUP=$GR --docdir=$DOCPATH --disable-mpi --with-cuda=$CUDA_ROOT $extra_disable


if [ "$1" == "install" ]; then
echo "------------------------"
echo " Compiling NO MPI"
echo "------------------------"

eval $EAR_FEATURES make -f Makefile.nompi.$tag $2
eval $EAR_FEATURES make -f Makefile.nompi.$tag install

fi
module unload $nvidia_modules
module unload $openmpi_modules
fi

if [ "$openmpi" == "enabled" ]; then
module load  $openmpi_modules
module load  $nvidia_modules

echo "------------------------"
echo "Configuring OpenMPI"
echo "------------------------"

my_cflags='-march=native -Wall'
echo "Using CFLAGS="$my_cflags
./configure --prefix=$INS_PATH EAR_TMP=$ETMP EAR_ETC=$ETC CC=gcc CC_FLAGS="$my_cflags" MPICC_FLAGS="$my_cflags" MPICC=mpicc MPI_VERSION=ompi MAKE_NAME=openmpi.$tag  USER=$US GROUP=$GR --docdir=$DOCPATH --with-cuda=$CUDA_ROOT $extra_disable


if [ "$1" == "install" ]; then
echo "------------------------"
echo " Compiling OpenMPI"
echo "------------------------"

eval $EAR_FEATURES make -f Makefile.openmpi.$tag $2
eval $EAR_FEATURES make -f Makefile.openmpi.$tag earl.install

fi
module unload $nvidia_modules
module unload $openmpi_modules
fi

echo "******************************** EAR installation ends here ********************************"
