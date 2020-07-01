CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

flop_OBJS = papi.o

######## RULES

all: $(flop_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) $(PAPI_CFLAGS) -c $< $(PAPI_LDFLAGS)

######## OPTIONS

install: ;

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra