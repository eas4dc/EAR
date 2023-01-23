# If USER_TEST is set to 1, any user can execute the hacks with the library.
CFLAGS  = -fPIC -O0 -I$(SRCDIR) -DUSER_TEST=0 $(CONSTANTS) 
LDFLAGS = -shared -ldl -lm -lpthread -rdynamic -pthread


ifeq ($(FEAT_MPI_API), 0)
LIBCC       = $(CC)
LIBCC_FLAGS = $(CC_FLAGS)
LIB_CFLAGS  = -DMPI=0
MPI_VERSION = gen
else
LIBCC       = $(MPICC)
LIBCC_FLAGS = $(MPICC_FLAGS)
LIB_CFLAGS  = $(MPI_CFLAGS) -DMPI=1 -DNODE_LB=1
endif


ifeq ($(FEAT_RESEARCH), 1)
	CFLAGS += -DMPI_OPTIMIZED=1 \
			  -DSINGLE_CONNECTION=0 \
			  -DEAR_DEF_TIME_GUIDED=0
	FEAT_EARL_LIGHT = 0
else
	CFLAGS += -DSINGLE_CONNECTION=1 \
			  -DMPI_OPTIMIZED=0 \
			  -DEAR_DEF_TIME_GUIDED=0
	FEAT_EARL_LIGHT = 0
endif


ifeq ($(FEAT_EARL_LIGHT), 1)
CFLAGS += -DEARL_LIGHT=1
else
CFLAGS += -DEARL_LIGHT=0
endif


# export LIBCC
# export LIBCC_FLAGS
export LIB_CFLAGS
export CFLAGS

########

ifdef MPI_VERSION
ext = ${MPI_VERSION}.so
else
ext = so
endif

libs_BINS  = libear.$(ext)
libs_PATH  = $(DESTDIR)/lib
libs_OBJS  = libear.so
libs_PERM  = 0755

######## FILES

comm_OBJS = \
    $(SRCDIR)/daemon/shared_configuration.o \
    $(SRCDIR)/daemon/powercap/powercap_status.o \
    $(SRCDIR)/report/report.o \
    $(SRCDIR)/management/libmgt.a \
    $(SRCDIR)/metrics/libmetrics.a \
    $(SRCDIR)/daemon/local_api/libeard.a \
    $(SRCDIR)/common/libcommon.a

core_OBJS = \
    api/ear.o \
    api/clasify.o \
    api/mpi_support.o \
    api/cupti.o \
    api/eard_dummy.o \
    common/library_shared_data.o \
    common/utils.o \
    metrics/metrics.o \
	metrics/energy_node_lib.o \
    models/models.o \
    models/cpu_power_model.o \
    policies/policy.o \
    policies/common/pc_support.o \
    policies/common/gpu_support.o \
    policies/common/cpu_support.o \
    policies/common/mpi_stats_support.o \
    policies/common/imc_policy_support.o \
    policies/common/generic.o \
    states/states_comm.o \
    states/states.o \
    tracer/tracer.o \
    dynais/dynais.a 


ifeq ($(FEAT_MPI_API), 1)
mpix_OBJS = \
    api/ear_mpi.o \
    api/mpic.o \
    api/mpif.o \
    common/global_comm.o
endif

######## RULES

all: all.components $(libs_OBJS)

%.so: $(mpix_OBJS) $(core_OBJS) $(comm_OBJS)
	$(LIBCC) $(LIBCC_FLAGS) -o $*.$(ext) $^ $(LDFLAGS)

######## OPTIONS

install: install.components libs.ginstall;

%.install: ;

clean: clean.components rclean;

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'api'        $* || exit
	@ $(MAKE) -C 'common'     $* || exit
	@ $(MAKE) -C 'dynais'     $* || exit
	@ $(MAKE) -C 'metrics'    $* || exit
	@ $(MAKE) -C 'models'     $* || exit
	@ $(MAKE) -C 'policies'   $* || exit
	@ $(MAKE) -C 'states'     $* || exit
	@ $(MAKE) -C 'tracer'     $* || exit
	@ $(MAKE) -C 'loader'     $* || exit

######## DEPENDENCIES

depend: depend.components;

depend-clean: depend-clean.components;

include $(SRCDIR)/Makefile.extra
