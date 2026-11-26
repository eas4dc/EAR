CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC -Wall

######## FILES

root_OBJS = \
    cpufreq.o \
	cpufreq_base.o \
    frequency.o \
	governor.o \
	priority.o

######## RULES

all: all.components $(root_OBJS)

# post compiles the hardware-free regressions; check executes them.
.PHONY: post check
ifeq ($(WITH_CMOCKA),1)
post: tests/cpufreq_regression_tests

check: post
	./tests/cpufreq_regression_tests

cpufreq_test_SOURCES = tests/cpufreq_regression_tests.c tests/test_support.c tests/mock_hardware.c \
    drivers/intel_pstate.c drivers/acpi_cpufreq.c archs/default.c governor.c \
    $(SRCDIR)/metrics/common/file.c $(SRCDIR)/common/utils/string.c \
    $(SRCDIR)/common/hardware/topology.c $(SRCDIR)/common/hardware/bithack.c
cpufreq_test_HEADERS = tests/cpufreq_fixture.h drivers/intel_pstate.h drivers/acpi_cpufreq.h \
    archs/default.h cpufreq.h governor.h Makefile
cpufreq_test_FLAGS = $(CFLAGS) $(CMOCKA_CPPFLAGS) -std=gnu11 -ffunction-sections -fdata-sections -pthread
cpufreq_test_LIBS = $(CMOCKA_LDFLAGS) $(CMOCKA_LIBS) -Wl,--gc-sections \
    -Wl,--wrap=open,--wrap=access,--wrap=close,--wrap=lseek,--wrap=pwrite,--wrap=pthread_mutex_lock
ifneq ($(CMOCKA_LIBDIR),)
cpufreq_test_LIBS += -Wl,-rpath,$(CMOCKA_LIBDIR)
endif

tests/cpufreq_regression_tests: $(cpufreq_test_SOURCES) $(cpufreq_test_HEADERS)
	$(CC) $(cpufreq_test_FLAGS) $(cpufreq_test_SOURCES) -o $@ $(cpufreq_test_LIBS)
else
post check:
	@echo "CPUFreq regression tests skipped: configure with --with-cmocka=<PATH> to enable them"
endif

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

test_prio: priority.o
	$(CC) $(CFLAGS) -o $@ $< ./archs/prio_dummy.o ./archs/prio_isst.o ./archs/prio_eard.o ../../daemon/lib  $(SRCDIR)/daemon/local_app/libeard.a $(SRCDIR)/metrics/libmetrics.a $(SRCDIR)/common/libcommon.a -lpthread

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'archs'    $* || exit
	@ $(MAKE) -C 'drivers'  $* || exit

######## OPTIONS

install: install.components

%.install: ;

clean: rclean clean.components
	rm -f tests/cpufreq_regression_tests

depend: depend.components

depend-clean: depend-clean.components

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
