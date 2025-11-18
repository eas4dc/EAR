########
OBJS = \
    metrics.o \
    energy_node_lib.o \
    fine_grain_collect.o


ifeq ($(DCGMI), 1)
OBJS += \
    dcgmi_lib.o \
    dcgmi_lib/turing.o \
    dcgmi_lib/hopper.o \
    dcgmi_lib/ampere.o \
    dcgmi_lib/common.o

dcgmilib_DEPS = dcgmi_lib/common.h

post_DEPS = dcgmi_lib_test
endif

ifeq ($(FEAT_DLB), 1)
OBJS += dlb_talp_lib.o
endif

# Variables for dcgmi_lib_test target
ifdef MPI_VERSION
	ext = .$(MPI_VERSION)
endif
dcgmi_lib_test_LDFLAGS = -L$(SRCDIR)/library -L$(SRCDIR)/library/loader
dcgmi_lib_test_LIBS = -lear$(ext) -learld -lpthread

all: $(OBJS)

dcgmi_lib/common.o: dcgmi_lib/common.c dcgmi_lib/common.h
	$(CC) $(CC_FLAGS) $(LIB_CFLAGS) $(CFLAGS) -c dcgmi_lib/common.c

dcgmi_lib/%.o: %.c %.h $(dcgmilib_DEPS)
	$(CC) $(CC_FLAGS) $(LIB_CFLAGS) $(CFLAGS) -c $<

dlb_talp_lib.o: dlb_talp_lib.c dlb_talp_lib.h
	$(CC) $(CC_FLAGS) $(LIB_CFLAGS) $(DLB_CPPFLAGS) $(CFLAGS) -c dlb_talp_lib.c

%.o: %.c %.h
	$(CC) $(CC_FLAGS) $(LIB_CFLAGS) $(CFLAGS) -c $<

post: $(post_DEPS)

dcgmi_lib_test: test/dcgmi_lib_test.c $(SRCDIR)/common/libcommon.a dcgmi_lib.h
	$(CC) $(CC_FLAGS) $(CFLAGS) -o $@ test/dcgmi_lib_test.c $(SRCDIR)/common/libcommon.a $(SRCDIR)/metrics/libmetrics.a $(dcgmi_lib_test_LDFLAGS) $(LDFLAGS) $(dcgmi_lib_test_LIBS) $(LIBS)

######## OPTIONS 

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
