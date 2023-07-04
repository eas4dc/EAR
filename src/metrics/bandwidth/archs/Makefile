CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

band_OBJS = \
    amd17.o \
    amd17df.o \
    amd19.o \
    dummy.o \
    eard.o \
    intel63.o \
    intel106.o \
    intel143.o \
	likwid.o \

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
