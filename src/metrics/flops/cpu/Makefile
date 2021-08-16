CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

flop_OBJS = \
    amd49.o \
    intel63.o \
    dummy.o

######## RULES

all: $(flop_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

papi.o: papi.c papi.h
	$(CC) $(CFLAGS) $(PAPI_CFLAGS) -c $<

######## OPTIONS

install: ;

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
