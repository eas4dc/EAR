CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

cycl_OBJS = papi.o

######## RULES

all: $(cycl_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) $(PAPI_CFLAGS) -c $< $(PAPI_LDFLAGS)

######## OPTIONS

install: ;

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra