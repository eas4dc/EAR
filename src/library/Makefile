MPI_API = 1
export MPI_API
ifdef MPI_API 
CFLAGS  = -fPIC -O3 -I$(SRCDIR) $(CONSTANTS) $(PAPI_CFLAGS) -pthread -DMPI=1
else
CFLAGS  = -fPIC -O3 -I$(SRCDIR) $(CONSTANTS) $(PAPI_CFLAGS) -pthread -DMPI=0
MPI_VERSION = seq
endif
export CFLAGS
LDFLAGS = -shared $(PAPI_LDFLAGS) -ldl -lm

########

ifdef MPI_VERSION
ext = ${MPI_VERSION}.so
else
ext = so
endif

ifdef MPI_CFLAGS
libs_BINS += libear.$(ext)
libs_PATH  = $(DESTDIR)/lib
libs_OBJS  = libear.so
libs_PERM  = 0755
endif

######## FILES

comm_OBJS = \
    $(SRCDIR)/daemon/eard_api.o \
    $(SRCDIR)/daemon/shared_configuration.o \
    $(SRCDIR)/metrics/libmetrics.a \
    $(SRCDIR)/common/libcommon.a

api_OBJS=api/ear.o

mpix_OBJS = \
    api/ear_mpi.o \
    api/ear_default.o \
    api/mpic.o \
    api/mpif.o \
		common/global_comm.o

core_OBJS = \
    metrics/metrics.o \
    models/models.o \
    policies/policy.o \
    states/states.o \
    states/states_periodic.o \
    tracer/tracer.o \
    dynais/dynais.a \
		common/library_shared_data.o 

######## RULES

all: all.components $(libs_OBJS)

ifdef MPI_API
%.so: $(api_OBJS)  $(mpix_OBJS) $(core_OBJS) $(comm_OBJS)
	$(MPICC) $(MPICC_FLAGS) -o $*.$(ext) $(api_OBJS) $(mpix_OBJS) $(core_OBJS) $(comm_OBJS) $(LDFLAGS)
else
%.so:  $(api_OBJS)  $(core_OBJS) $(comm_OBJS)
	$(CC) $(CC_FLAGS) -o $*.$(ext) $(api_OBJS) $(core_OBJS) $(comm_OBJS) $(LDFLAGS)
endif

######## OPTIONS

install: install.components libs.ginstall;

%.install: ;

clean: clean.components rclean;

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'api'        $* || exit
	@ $(MAKE) -C 'dynais'     $* || exit
	@ $(MAKE) -C 'loader'     $* || exit
	@ $(MAKE) -C 'metrics'    $* || exit
	@ $(MAKE) -C 'models'     $* || exit
	@ $(MAKE) -C 'policies'   $* || exit
	@ $(MAKE) -C 'states'     $* || exit
	@ $(MAKE) -C 'tracer'     $* || exit
	@ $(MAKE) -C 'common'     $* || exit

######## DEPENDENCIES

depend: depend.components;

depend-clean: depend-clean.components;

include $(SRCDIR)/Makefile.extra
