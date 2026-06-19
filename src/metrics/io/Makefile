CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

io_OBJS = \
    io.o \
    io_old.o

######## RULES

all: all.components $(io_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'archs' $* || exit

######## OPTIONS

install: ;

%.install: ;

clean: rclean clean.components;

depend: 

depend-clean:

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
