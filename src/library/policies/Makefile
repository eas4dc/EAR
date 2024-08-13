all: all.components policy.o policy_plugin.o policy_ctx.o
 
%.o: %.c 
	$(CC) $(CC_FLAGS) $(LIB_CFLAGS) $(CFLAGS) -c $<

post: p_plugin_test

p_plugin_test: test.o policy_plugin.o
	$(CC) $(CC_FLAGS) $(LIB_CFLAGS) $(CLFAGS) -o $@ test.o policy_plugin.o ../../common/libcommon.a -ldl

test.o: test/test.c
	$(CC) $(CC_FLAGS) $(LIB_CFLAGS) $(CFLAGS) -c test/test.c

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
