CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

temp_OBJS = \
    amd17.o \
    intel63.o \
	dummy.o

######## RULES

all: $(temp_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
