CFLAGS = $(CC_FLAGS) -fPIC -I$(SRCDIR) $(CONSTANTS)

######## INSTALLATION

api_BINS = libear_api.a
api_PATH = $(DESTDIR)/lib
api_PERM = 0664
oths_BINS = ear.h
oths_PATH = $(DESTDIR)/include
oths_PERM = 0664

######## RULES

all: app_server_api.o $(api_BINS)

libear_api.a: app_api.o
	$(AR) rcs $@ app_api.o

app_api.o: app_api.c ear.h
	$(CC) $(CFLAGS)	-c $<

app_server_api.o: app_server_api.c app_server_api.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: api.ginstall oths.ginstall;

devel.install: api.ginstall oths.ginstall;

%.install: ;	

clean: api.gclean rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
