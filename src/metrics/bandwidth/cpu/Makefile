CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

band_OBJS = intel_haswell.o utils.o

######## RULES

all: $(band_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: ;

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
