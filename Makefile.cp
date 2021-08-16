# Features
FEAT_MPI_API = 1
FEAT_AVX512  = 1

# Compilers
CC           = /hpc/base/ctt/packages/compiler/gnu/9.1.0/bin/gcc
CC_FLAGS     =
MPICC        = /hpc/base/intel/parallel_studio_xe_2018.2.046/compilers_and_libraries_2018/linux/mpi/intel64/bin/mpiicc
MPICC_FLAGS  = 

# Includes
MPI_BASE     = /hpc/base/intel/parallel_studio_xe_2018.2.046/compilers_and_libraries_2018/linux/mpi/intel64
MPI_CFLAGS   = -I$(MPI_BASE)/include
MPI_VERSION  = 
GSL_BASE     = /hpc/base/ctt/packages/gsl/1.16
GSL_CFLAGS   = -I/hpc/base/ctt/packages/gsl/1.16/include
GSL_LDFLAGS  = -L/hpc/base/ctt/packages/gsl/1.16/lib -lm -lgsl -lgslcblas
SLURM_BASE   = /usr
SLURM_CFLAGS = -I/usr/include
DB_BASE      = /usr
DB_CFLAGS    = 
DB_LDFLAGS   = -L/usr/lib64/mysql -lmysqlclient
CUDA_BASE    = /hpc/base/cuda/cuda-10.1
#PAPI_BASE    = /hpc/base/ctt/packages/papi/5.7.0
#PAPI_CFLAGS  = -I/hpc/base/ctt/packages/papi/5.7.0/include
#PAPI_LDFLAGS = -L/hpc/base/ctt/packages/papi/5.7.0/lib -Wl,-rpath,/hpc/base/ctt/packages/papi/5.7.0/lib -lpapi

# Directories
ROOTDIR      = $(shell pwd)
SRCDIR       = $(ROOTDIR)/src
DESTDIR      = /home/xjaneas/my_ear
ETCDIR       = /home/xjaneas/my_ear/etc
TMPDIR       = /var/ear
DOCDIR       = /share/doc/ear

# Installation
CONSTANTS    = -DSEC_KEY=10001
CHOWN_USR    = xjaneas
CHOWN_GRP    = 
REPLACE      =

######## EXPORTS

include ./Makefile.exports

######## RULES

.PHONY: check functionals

all:
	@ $(MAKE) -C 'src' all || exit

full: clean all 

check:
	@ $(MAKE) -C 'src' check

install:
	@ $(MAKE) -C 'etc' install || exit
	@ $(MAKE) -C 'src' install || exit

clean:
	@ $(MAKE) -C 'src' clean || exit

########

devel.install: install;
	@ $(MAKE) -C 'etc' devel.install || exit
	@ $(MAKE) -C 'src' devel.install || exit

etc.install: ;
	@ $(MAKE) -C 'etc' etc.install || exit
	@ $(MAKE) -C 'src' etc.install || exit

rpm.install: install;
	@ $(MAKE) -C 'etc' rpm.install || exit

scripts.install: ;
	@ $(MAKE) -C 'etc' scripts.install || exit

doc.install: ;
	@ $(MAKE) -C 'doc' install || exit 

%.build: ;
	@ $(MAKE) -C 'src' $*.build || exit

%.install: ;
	@ $(MAKE) -C src $*.install || exit

########

depend:
	@ $(MAKE) -C 'src' depend || exit

depend-clean:
	@ $(MAKE) -C 'src' depend-clean || exit
