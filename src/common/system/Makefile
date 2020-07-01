CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC $(CONSTANTS) -O2

######## INSTALLATION

libc_OBJS = $(patsubst %.c, %.o, $(shell find -maxdepth 1 -type f -name '*.c'))

######## FILES

######## RULES

all: $(libc_OBJS)

random.o: random.c random.h
	$(CC) $(CFLAGS) -c $< -mrdrnd

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

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
