all: all.components policy.o
 
%.o: %.c 
	$(CC) $(CC_FLAGS) $(CFLAGS) $(LIB_CFLAGS) -c $<

######## OPTIONS 
.PRECIOUS: %.o

install: install.components;

clean: clean.components rclean;

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'common'         $* || exit
	@ $(MAKE) -C 'eard_plugins'   $* || exit
	@ $(MAKE) -C 'policy_plugins' $* || exit

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
