CFLAGS = $(CC_FLAGS) -fPIC -I$(SRCDIR) $(CONSTANTS) $(DB_CFLAGS)

######## FILES

ext_OBJS = \
		   $(SRCDIR)/common/external/ear_external.o \
		   $(SRCDIR)/common/system/shared_areas.o
		   

######## RULES

all: $(ext_OBJS) 

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: 

%.install: ;

clean: rclean;



######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra

