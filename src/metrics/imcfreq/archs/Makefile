CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC -Wall -g

######## FILES

freq_OBJS = \
    amd17.o \
    intel63.o \
    dummy.o \
    eard.o

######## RULES

all: $(freq_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: ;

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
