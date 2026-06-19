CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

io_OBJS = \
    dummy.o \
    proc_file.o

######## RULES

all: $(io_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: ;

%.install: ;

clean: rclean;

depend: ;

depend-clean: ;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
