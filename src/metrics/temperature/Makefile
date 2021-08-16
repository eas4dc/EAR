CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

temp_OBJS = temperature.o

######## RULES

all: all.components $(temp_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<
#	$(CC) $(CFLAGS) -o temp temperature.o $(SRCDIR)/metrics/libmetrics.a $(SRCDIR)/common/libcommon.a

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'archs' $* || exit

######## OPTIONS

install: install.components;

%.install: ;

clean: rclean clean.components;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
