CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

proc_OBJS = \
    stat_file.o \
    dummy.o

######## RULES

all: $(proc_OBJS)

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
