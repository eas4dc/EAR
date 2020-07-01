CFLAGS = $(CC_FLAGS) -fPIC -I$(SRCDIR) $(CONSTANTS) 

######## FILES

comm_OBJS = \
    msr.o \
    pci.o \
    papi.o

######## RULES

all: $(comm_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

papi.o: papi.c papi.h
	$(CC) $(CFLAGS) $(PAPI_CFLAGS) -c $< 

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'cpu' $* || exit

######## OPTIONS

install: ;

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
