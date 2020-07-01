CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

band_OBJS = \
    power_metrics.o 

######## RULES

all: all.components $(band_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## SUBFOLDERS

%.components: ;

######## OPTIONS

install: ;

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
