CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

cycl_OBJS = \
    intel63.o \
    dummy.o

######## RULES

all: $(cycl_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

papi.o: papi.c papi.h
	$(CC) $(CFLAGS) $(PAPI_CFLAGS) -c $<

######## OPTIONS

install: ;

%.install: ;

clean: rclean;

#depend: ;

#depend-clean: ;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
