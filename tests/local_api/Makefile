CC = /usr/bin/gcc

# WF_SUPPORT must match the tree these tests link against: it adds job_t.local_id
# (src/common/types/job.h), so a mismatch silently changes the layout of
# application_t and every struct carrying a job - including the daemon_req that
# eards_connect() puts on the wire. src/Makefile derives it from FEAT_WF_SUPPORT
# in the top-level Makefile.in; override on the command line if that changes.
WF_SUPPORT ?= 1

CFLAGS = -g -O0 -rdynamic -Xlinker -z -Xlinker muldefs -DWF_SUPPORT=$(WF_SUPPORT)
SRCDIR ?= ../../src
INCLUDE = -I$(SRCDIR)

TESTS = rpc_answer_size_test

rpc_answer_size_test_SRC = rpc_answer_size_test.c

EAR_ARCHIVES = \
	$(SRCDIR)/daemon/local_api/libeard.a \
	$(SRCDIR)/metrics/libmetrics.a \
	$(SRCDIR)/management/libmgt.a \
	$(SRCDIR)/common/libcommon.a

rpc_answer_size_test_LIBS = -Wl,--whole-archive $(EAR_ARCHIVES) -Wl,--no-whole-archive -ldl -lpthread -lm

.PHONY: all run_tests clean

all: $(TESTS)

# The %.c and $(EAR_ARCHIVES) prerequisites matter: without them make treats an
# existing binary as up to date and silently runs a stale build after an edit -
# including after a rebuild of the archive holding the code under test.
$(TESTS): %: %.c $(EAR_ARCHIVES)
	@echo "Compiling $@..."
	$(CC) $(CFLAGS) $(INCLUDE) $(eval src=$$($@_SRC)) $(eval libs=$$($@_LIBS)) $(src) -o $@ $(libs)

run_tests: $(TESTS)
	@echo "Running tests..."
	@passed=0; failed=0; \
	for test in $(TESTS); do \
		echo "Running $$test..."; \
		./$$test; \
		if [ $$? -eq 0 ]; then \
			echo "$$test passed."; \
			passed=$$((passed + 1)); \
		else \
			echo "$$test failed."; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo "Tests completed. Total passed: $$passed, Total failed: $$failed."

clean:
	rm -f $(TESTS)
