CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

temp_OBJS = temperature.o

######## RULES

all:  $(temp_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## SUBFOLDERS


######## OPTIONS

install: ;

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
