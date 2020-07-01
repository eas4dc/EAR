CFLAGS  = -fPIC -O3 -I$(SRCDIR) $(CONSTANTS) $(PAPI_CFLAGS)
LDFLAGS = -shared $(PAPI_LDFLAGS) -ldl
######## RULES

tracer_BINS = \
	tracer_paraver.so
tracer_PATH = $(DESTDIR)/lib/plugins/tracer
tracer_OBJ = \
  tracer_paraver.o
tracer_PERM = 0775

all: $(tracer_OBJS) $(tracer_BINS)

allt: DEFINES=-DEAR_GUI
allt: tracer.o tracer_dynais.o tracer_paraver.o

%.o: %.c %.h
	$(MPICC) $(CC_FLAGS) $(CFLAGS) $(DEFINES) -c $<

%.so:%.o
	$(MPICC) $(CC_FLAGS) $(CFLAGS) $(DEFINES) $(LDFLAGS)  -shared $< -o $@

tracer_cache.so:tracer_cache.o $(SRCDIR)/metrics/cache/cpu/papi.o
	$(MPICC) $(CC_FLAGS) $(CFLAGS) $(DEFINES) $(LDFLAGS) $(SRCDIR)/metrics/cache/cpu/papi.o  -shared $< -o $@


######## OPTIONS

install: tracer.ginstall;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
