CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

gpu_mgt_OBJS = \
  	gpu_mgt.o 

######## RULES

all:$(gpu_mgt_OBJS)  

%.o: %.c
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: ;
%.install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
