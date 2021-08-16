CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

cach_OBJS = perf.o

######## RULES

all: $(cach_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: ;

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
