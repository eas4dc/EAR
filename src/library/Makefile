# If USER_TEST is set to 1, any user can execute the hacks with the library.
CFLAGS  = -fPIC  -I$(SRCDIR) -DUSER_TEST=0 $(CONSTANTS)
LDFLAGS = -shared -ldl -lm -lpthread -rdynamic -pthread


ifeq ($(FEAT_MPI_LIBRARY), 0)
LIBCC       = $(CC)
LIBCC_FLAGS = $(CC_FLAGS)
LIB_CFLAGS  = -DMPI=0 -DNODE_LB=1
MPI_VERSION = gen
else
LIBCC       = $(MPICC)
LIBCC_FLAGS = $(MPICC_FLAGS)
LIB_CFLAGS  = $(MPI_CFLAGS) -DMPI=1 -DNODE_LB=1
CFLAGS     += -DEAR_DEF_TIME_GUIDED=0 -DMPI_SAMPLING=1 -DMPI_STATS=1

## MPI_SAMPLING can be used to do an MPI statistic sampling
## MIX_RPM implements a blocking operation to read in rpcs
endif

ifeq ($(FEAT_RESEARCH), 1)
CFLAGS += -DEAR_DEF_TIME_GUIDED=0
endif

ifeq ($(FEAT_EAR_OFF), 1)
CFLAGS += -DEAR_OFF=1
endif

ifeq ($(FEAT_EARL_LIGHT), 1) 
CFLAGS += -DEARL_LIGHT=1
else ifeq ($(FEAT_EARL_LITE), 1)
CFLAGS += -DEARL_LIGHT=1
else
CFLAGS += -DEARL_LIGHT=0
endif

ifeq ($(DCGMI), 1)
CFLAGS += -DDCGMI=1
else
CFLAGS += -DDCGMI=0
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
    metrics/fine_grain_collect.o \
    models/cpu_power_model.o \
    models/energy_model.o \
    policies/policy.o \
    policies/policy_ctx.o \
    policies/common/pc_support.o \
    policies/common/gpu_support.o \
    policies/common/cpu_support.o \
    policies/common/mpi_stats_support.o \
    policies/common/imc_policy_support.o \
    policies/common/cpuprio_support.o \
    policies/common/generic.o \
    states/states_comm.o \
    states/states.o \
    tracer/tracer.o \
    dynais/dynais.a

ifeq ($(DCGMI), 1)
core_OBJS += \
    metrics/common.o \
    metrics/dcgmi_lib.o \
    metrics/turing.o \
    metrics/hopper.o \
    metrics/ampere.o
endif

ifeq ($(FEAT_DLB), 1)
core_OBJS += metrics/dlb_talp_lib.o
endif

ifeq ($(FEAT_MPI_LIBRARY), 1)
mpix_OBJS = \
    api/ear_mpi.o \
    api/mpic.o \
    api/mpif.o \
    common/global_comm.o
endif

######## RULES

all: all.components $(libs_OBJS) 

%.so: $(mpix_OBJS) $(core_OBJS) $(comm_OBJS)
	$(LIBCC) $(LIBCC_FLAGS) $(DLB_LDFLAGS) -o $*.$(ext) $^ $(LDFLAGS) $(DLB_LIBS)

######## OPTIONS

install: install.components libs.ginstall ;

%.install: ;

clean: clean.components rclean;

post:
	@ $(MAKE) -C 'metrics' $@ || exit
	@ $(MAKE) -C 'policies' $@ || exit

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'loader'     $* || exit
	@ $(MAKE) -C 'api'        $* || exit
	@ $(MAKE) -C 'common'     $* || exit
	@ $(MAKE) -C 'dynais'     $* || exit
	@ $(MAKE) -C 'metrics'    $* || exit
	@ $(MAKE) -C 'models'     $* || exit
	@ $(MAKE) -C 'policies'   $* || exit
	@ $(MAKE) -C 'states'     $* || exit
	@ $(MAKE) -C 'tracer'     $* || exit

######## DEPENDENCIES

depend: depend.components;

depend-clean: depend-clean.components;

include $(SRCDIR)/Makefile.extra
