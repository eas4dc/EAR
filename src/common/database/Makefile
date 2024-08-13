CFLAGS = $(CC_FLAGS) -fPIC -I$(SRCDIR) $(CONSTANTS) $(DB_CFLAGS) -Wall -Wextra 

######## FILES

ifdef DB_BASE
dtbs_OBJS = \
    db_helper.o \
    postgresql_io_functions.o
endif

ifeq ($(FEAT_WF_SUPPORT), 1) 
ifdef DB_BASE
  dtbs_OBJS += mysql_io_functions.o
  endif
endif

######## RULES

all: $(dtbs_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: ;

%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
