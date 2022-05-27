CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

band_OBJS = \
    eard.o \
    dummy.o \
    bypass.o \
    intel63.o \
    intel106.o \
    amd17.o \
	likwid.o

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
