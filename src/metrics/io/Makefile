CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

io_OBJS = io.o

######## RULES

all: $(io_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## SUBFOLDERS


######## OPTIONS

install: ;

%.install: ;

clean: rclean ;

depend: 

depend-clean:

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
