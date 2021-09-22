CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

stal_OBJS = stalls.o

######## RULES

all: all.components $(stal_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## SUBFOLDERS

%.components: ;
#	@ $(MAKE) -C 'cpu' $* || exit

######## OPTIONS

install: ;

%.install: ;

clean: rclean clean.components;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
