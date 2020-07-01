CFLAGS = $(CC_FLAGS) -fPIC -I$(SRCDIR) $(CONSTANTS)

######## FILES

conf_OBJS = policy_conf.o \
    node_conf.o \
    cluster_conf.o \
    cluster_conf_read.o \
    cluster_conf_verbose.o

######## RULES

all: $(conf_OBJS)

%.o: %.c cluster_conf.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install:

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
