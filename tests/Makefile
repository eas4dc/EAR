SRCDIR = ../src
CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC $(CONSTANTS) -g -O0 -Wall
LDFLAGS = -lm
CCC     = $(CC) $(CFLAGS) 

comm_OBJS = \
    $(SRCDIR)/common/libcommon.a

mang_OBJS = \
    $(SRCDIR)/management/libmgt.a \
	$(SRCDIR)/metrics/libmetrics.a 

# Object files for the test
# Source files for the test dependencies
TEST_DEPS = $(SRCDIR)/daemon/powercap/powercap_status.c \
	$(SRCDIR)/daemon/shared_configuration.c

all: test_powercap test_powercap_monitor

test_powercap: test_powercap.c 
	$(CCC) -o $@ $^ $(TEST_DEPS) $(LDFLAGS) $(mang_OBJS) $(SRCDIR)/daemon/local_api/libeard.a $(comm_OBJS) -lpthread

test_powercap_monitor: test_powercap_monitor.c
	$(CCC) -o $@ $^ $(LDFLAGS) -lpthread

clean:
	rm -f test_powercap test_powercap_monitor
