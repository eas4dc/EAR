CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC $(CONSTANTS)

ifeq ($(ARCH), X86)
CFLAGS_STRS = -mavx2
endif

######## INSTALLATION

libc_OBJS = $(patsubst %.c, %.o, $(shell find -maxdepth 1 -type f -name '*.c'))

######## FILES

######## RULES

all: $(libc_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

stress.o: stress.c stress.h
	$(CC) $(CFLAGS) $(CFLAGS_STRS) -DARCH_$(ARCH) -c $<

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
