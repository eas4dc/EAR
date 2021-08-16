CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

freq_OBJS = \
	intel63.o\
	intel_dummy.o

######## RULES

all: $(freq_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: ;

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
