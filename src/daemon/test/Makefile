############################################################################
# Copyright (c) 2026 Energy Aware Solutions, S.L
############################################################################

SRCDIR ?= $(abspath ../..)
DAEMON_DIR ?= $(abspath ..)

CFLAGS = $(CC_FLAGS) -fPIC -I$(SRCDIR) $(CONSTANTS) $(CMOCKA_CPPFLAGS)
PM_TEST_OBJ = power_monitor_test.o

TEST_BINS = \
	test_power_monitor_locks \
	test_power_monitor_concurrency \
	test_powercap_context_locks

POWERCAP_MGT_TEST_OBJ = powercap_mgt_test.o
POWERCAP_TEST_OBJ = powercap_test.o


.PHONY: all check clean

ifeq ($(WITH_CMOCKA), 1)
all: $(TEST_BINS)

$(PM_TEST_OBJ): $(DAEMON_DIR)/power_monitor.c
	$(CC) $(CFLAGS) -ffunction-sections -fdata-sections -DUNIT_TEST  -c $< -o $@

test_power_monitor_locks: test_power_monitor_locks.c $(DAEMON_DIR)/power_monitor_lock.c
	$(CC) $(CFLAGS) -o $@ \
		test_power_monitor_locks.c \
		$(DAEMON_DIR)/power_monitor_lock.c \
		$(CMOCKA_LDFLAGS) $(CMOCKA_LIBS) \
		-lpthread -lm $(LDFLAGS)

test_power_monitor_concurrency: test_power_monitor_concurrency.c $(PM_TEST_OBJ)
	$(CC) $(CFLAGS) -DUNIT_TEST \
		-Wl,--gc-sections \
		-o $@ \
		test_power_monitor_concurrency.c \
		$(PM_TEST_OBJ) \
		$(DAEMON_DIR)/power_monitor_lock.c \
		$(SRCDIR)/common/libcommon.a \
		$(CMOCKA_LDFLAGS) $(CMOCKA_LIBS) \
		-lpthread -lm $(LDFLAGS)

$(POWERCAP_MGT_TEST_OBJ): $(DAEMON_DIR)/powercap/powercap_mgt.c
	$(CC) $(CFLAGS) -DUNIT_TEST \
		-ffunction-sections -fdata-sections \
		-c $< -o $@

$(POWERCAP_TEST_OBJ): $(DAEMON_DIR)/powercap/powercap.c
	$(CC) $(CFLAGS) -DUNIT_TEST \
		-ffunction-sections -fdata-sections \
		-c $< -o $@

test_powercap_context_locks: test_powercap_context_locks.c $(POWERCAP_MGT_TEST_OBJ) $(PM_TEST_OBJ) $(POWERCAP_TEST_OBJ)
	$(CC) $(CFLAGS) -DUNIT_TEST \
		-Wl,--gc-sections \
		-o $@ \
		test_powercap_context_locks.c \
		$(POWERCAP_MGT_TEST_OBJ) \
		$(POWERCAP_TEST_OBJ) \
		$(PM_TEST_OBJ) \
		$(DAEMON_DIR)/power_monitor_lock.c \
		$(SRCDIR)/common/libcommon.a \
		$(CMOCKA_LDFLAGS) $(CMOCKA_LIBS) \
		-lpthread -lm $(LDFLAGS)

check: all
	./test_power_monitor_locks
	./test_power_monitor_concurrency
	./test_powercap_context_locks
else
all:
check: all;
endif

clean:
	rm -f $(TEST_BINS) $(PM_TEST_OBJ) $(POWERCAP_MGT_TEST_OBJ) $(POWERCAP_TEST_OBJ) *.o


-include $(SRCDIR)/Makefile.extra
