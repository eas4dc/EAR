CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

rapl_OBJS = \
    intel63.o

######## RULES

all: $(rapl_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
