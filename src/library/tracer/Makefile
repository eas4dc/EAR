LDFLAGS = -ldl

######## RULES

tracer_OBJS = \
    tracer.o \
    tracer_paraver.o
tracer_BINS = tracer_paraver.so
tracer_DEPS = $(SRCDIR)/common/libcommon.a
tracer_PATH = $(DESTDIR)/lib/plugins/tracer
tracer_PERM = 0775

########

all: $(tracer_OBJS) $(tracer_BINS)

%.o: %.c %.h
	$(CC) $(CC_FLAGS) $(CFLAGS) -c $<

%.so: %.o $(tracer_DEPS)
	$(CC) $(CC_FLAGS) -shared -o $@ $^ $(LDFLAGS)

######## OPTIONS

install: tracer.ginstall;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
