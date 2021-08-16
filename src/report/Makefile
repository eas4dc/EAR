CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC -Wall -g

######## FILES

repo_OBJS = \
	report.o
repo_BINS = \
	eard.so \
	csv.so \
	csv_ts.so \
	plug_dummy.so
repo_PERM = 0775
repo_PATH = $(DESTDIR)/lib/plugins/reports


######## RULES

all: $(repo_OBJS) $(repo_BINS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

%.so: %.c
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)

######## SUBFOLDERS

%.components: ;

######## OPTIONS

install: repo.ginstall

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
