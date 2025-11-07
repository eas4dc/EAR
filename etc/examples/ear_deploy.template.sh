#!/bin/bash

####################### Configuration starts here ####################### 

## Add here the external features you want to enable in all the cases. 


# Specify source path
SRCPATH=$PWD

# EAR library versions: enabled or disabled
openmpi="enabled"
nompi="enabled"
intelmpi="enabled"


# Installation paths
export EAR_VERSION=6.0
# tag is a text to be included as prefix for the installation path
export tag=""
# Installation prefix path
export INSTALL_ROOT=/opt/EAR/
export INS_PATH=$INSTALL_ROOT/ear$EAR_VERSION.$tag

export ETC=$INS_PATH/etc
export ETMP=/var/ear
export DOCPATH=$INSTALL_ROOT/doc


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


# Ear deploy script

# Modify with specific modules to be loaded
core_modules="base-env eb-env binutils/2.40 Prefix/Production GCCcore"
intel_modules="iimpi imkl"
nvidia_modules=""
openmpi_modules="gompi"

## If some specific paths must be defined becauset there is no module, add here
export SLURMPATH=/usr
export CUDA_ROOT=/hpc/base/EasyBuild/EL9-ICX/software/CUDA/12.2.0


####################### Configuration ends here ####################### 

# Start preparing environment

echo "******************************** EAR installation starts here ********************************"
module purge
module load $core_modules

# PATH configuration
cd $SRCPATH

echo "***** Autoreconfig ****"
autoreconf -i



if [ "$nompi" == "enabled" ]; then
module load  $openmpi_modules
module load  $nvidia_modules

echo "------------------------"
echo "Configuring NO MPI"
echo "------------------------"


# With gcc
my_cflags='-march=native -Wall -g'

echo "Using CFLAGS="$my_cflags
./configure --prefix=$INS_PATH EAR_TMP=$ETMP EAR_ETC=$ETC CC=gcc CC_FLAGS="$my_cflags" MPICC_FLAGS="$my_cflags" MPICC=mpicc MAKE_NAME=nompi.$tag  USER=$US GROUP=$GR --docdir=$DOCPATH --disable-mpi --with-cuda=$CUDA_ROOT $extra_disable --with-slurm=$SLURMPATH


echo "------------------------"
echo " Compiling NO MPI"
echo "------------------------"

eval $EAR_FEATURES make -f Makefile.nompi.$tag full
eval $EAR_FEATURES make -f Makefile.nompi.$tag install
eval $EAR_FEATURES make -f Makefile.intelmpi.$tag etc.install
eval $EAR_FEATURES make -f Makefile.intelmpi.$tag doc.install

module unload $nvidia_modules
module unload $openmpi_modules
fi

## Intel MPI is the default library, for other librarys a specific extension is needed.
## Use MPI_VERSION=[ompi | fujitsu | cray]
if [ "$openmpi" == "enabled" ]; then
module load  $openmpi_modules
module load  $nvidia_modules

echo "------------------------"
echo "Configuring OpenMPI"
echo "------------------------"

my_cflags='-march=native -Wall -g'
echo "Using CFLAGS="$my_cflags
./configure --prefix=$INS_PATH EAR_TMP=$ETMP EAR_ETC=$ETC CC=gcc CC_FLAGS="$my_cflags" MPICC_FLAGS="$my_cflags" MPICC=mpicc MPI_VERSION=ompi MAKE_NAME=openmpi.$tag  USER=$US GROUP=$GR --docdir=$DOCPATH --with-cuda=$CUDA_ROOT $extra_disable --with-slurm=$SLURMPATH


echo "------------------------"
echo " Compiling OpenMPI"
echo "------------------------"

eval $EAR_FEATURES make -f Makefile.openmpi.$tag full
eval $EAR_FEATURES make -f Makefile.openmpi.$tag install

module unload $nvidia_modules
module unload $openmpi_modules
fi

if [ "$intelmpi" == "enabled" ]; then
module load  $intel_modules
module load  $nvidia_modules

echo "------------------------"
echo "Configuring IntelMPI"
echo "------------------------"

my_cflags='-Wno-unused-command-line-argument -static-intel -Wall -g -Rno-debug-disables-optimization -Wno-empty-body'
echo "Using CFLAGS="$my_cflags

./configure --prefix=$INS_PATH EAR_TMP=$ETMP EAR_ETC=$ETC CC=icx CC_FLAGS="$my_cflags" MPICC_FLAGS="$my_cflags" MPICC=mpiicx MAKE_NAME=intelmpi.$tag   USER=$US GROUP=$GR --docdir=$DOCPATH --with-cuda=$CUDA_ROOT $extra_disable --with-slurm=$SLURMPATH

echo "------------------------"
echo " Compiling IntelMPI"
echo "------------------------"

eval $EAR_FEATURES make -f Makefile.intelmpi.$tag full
eval $EAR_FEATURES make -f Makefile.intelmpi.$tag install

module unload $nvidia_modules
module unload $intel_modules
fi
echo ""
echo "******************************** EAR deployment done ********************************"
echo ""

ls -ltrR $INS_PATH

echo ""
echo "******************************** next steps ********************************"
echo ""


echo "For a full installation, execute the following steps"
echo "1- Prepare ear.conf file"
echo "	Templates files have been installed for ear.conf, services files, ear.plugstack.conf etc. "
echo "	Copy $EAR_ETC/ear/ear.conf.full.template in $EAR_ETC/ear/ear.conf and update it with your cluster conf"
echo "2 - Create the EAR DB"
echo "	define \"export EAR_ETC=$EAR_ETC\""
echo "	execute \"$INS_PATH/sbin/edb_create -r\" to create the DB"
echo "3- Deploy $EAR_ETC/systemd/*.service files in /usr/lib/systemd/system/eard.service"
echo "4- Enable services \"systemctl enable eard\" and \"systemctl start eard\""
echo "5- Enable ear plugin"
echo "6- Use it \"sbatch --ear=on my_job.sh\""


echo "******************************** EAR installation ends here ********************************"
