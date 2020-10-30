CFLAGS = $(CC_FLAGS) -I$(SRCDIR) $(DB_FLAGS) -lm
LDFLAGS = -lm

######## FILES

oths_DEPS = \
        $(SRCDIR)/common/libcommon.a 
oths_BINS = \
    ips_check \
    postgres_test \
    temperature_check

test_BINS = ear_conf \
			propagation_test
test_PATH = $(DESTDIR)/bin/tests
test_PERM = 0770

scrp_BINS = \
    dummy_energycap_action.sh 
scrp_PATH = $(DESTDIR)/bin/tests
scrp_PERM = 0770

######## RULES

all: $(oths_BINS) $(test_BINS)

postgres_test: postgres_test.c $(oths_DEPS)
	$(CC) $(CFLAGS) -o postgres_test $(DB_LDFLAGS) $^ $(LDFLAGS)

temperature_check: temperature_check.c $(SRCDIR)/metrics/libmetrics.a $(oths_DEPS) 
	$(CC) $(CFLAGS) -o temperature_check $^ $(LDFLAGS)

propagation_test: propagation_test.c $(oths_DEPS)
	$(CC) $(CFLAGS) -o propagation_test $^ $(LDFLAGS)

ear_conf: ear_conf.c $(oths_DEPS)
	$(CC) $(CFLAGS) -o ear_conf $^ $(LDFLAGS)

ips_check: ips_check.c $(oths_DEPS)
	$(CC) $(CFLAGS) -o ips_check $^ $(LDFLAGS)

######## OPTIONS

install: test.ginstall scrp.ginstall;

clean: oths.gclean test.gclean rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
