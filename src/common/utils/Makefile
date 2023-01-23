CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC $(CONSTANTS)

######## INSTALLATION

libc_OBJS = $(patsubst %.c, %.o, $(shell find -maxdepth 1 -type f -name '*.c'))

######## FILES

######## RULES

all: $(libc_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

stress.o: stress.c stress.h
	$(CC) $(CFLAGS) -mavx2 -c $<

######## OPTIONS

install: ;

devel.install: ;

%.install: ;

clean: rclean;

######## SUBFOLDERS

%.components: ;

######## DEPENDENCIES

depend: depend.components

depend-clean: depend-clean.components

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
