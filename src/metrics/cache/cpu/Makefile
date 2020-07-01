CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

cach_OBJS = papi.o

######## RULES

all: $(cach_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) $(PAPI_CFLAGS) -c $< $(PAPI_LDFLAGS)

######## OPTIONS

install: ;

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
