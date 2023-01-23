CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC -Wall

######## FILES

freq_OBJS = \
	imcfreq.o

######## RULES

all: all.components $(freq_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'archs' $* || exit

######## OPTIONS

install: ;

%.install: ;

clean: rclean clean.components;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
