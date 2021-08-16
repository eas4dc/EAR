ifeq ($(FEAT_GPU_OPT), 1)
GPU_OPT_FLAG = -DGPU_OPT=1
endif

######## FILES

supp_OBJS = \
    cpu_support.o \
    gpu_support.o \
    imc_policy_support.o \
    mpi_stats_support.o \
    pc_support.o \
	generic.o

########

.PRECIOUS: %.o

all: $(supp_OBJS)
 
%.o: %.c 
	$(CC) $(CC_FLAGS) $(CFLAGS) $(LIB_CFLAGS) $(GPU_OPT_FLAG) -c $<

######## OPTIONS 

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
