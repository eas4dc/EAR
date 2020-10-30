CFLAGS = $(CC_FLAGS) -fPIC -I$(SRCDIR) $(CONSTANTS)

######## FILES

conf_OBJS = policy_conf.o \
    node_conf.o \
		cluster_conf_eargm.o \
		cluster_conf_eard.o \
		cluster_conf_eardbd.o \
		cluster_conf_generic.o \
		cluster_conf_earlib.o \
		cluster_conf_etag.o \
		cluster_conf_tag.o \
		cluster_conf_db.o \
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
