CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC -Wall

######## OPTIONS

RECOVERY = 0

######## FILES

user_OBJS = \
    cpufreq_user.o
root_OBJS = \
    cpufreq.o \
    frequency.o \
	governor.o
ifeq ($(RECOVERY), 1)
root_OBJS += \
    replace
endif

######## RULES

all: all.components $(root_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

replace: $(root_OBJS)
	mv recovery/frequency.o frequency.o
	mv recovery/cpupower.o cpufreq.o

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'archs'    $* || exit
	@ $(MAKE) -C 'drivers'  $* || exit
ifeq ($(RECOVERY), 1)
	@ $(MAKE) -C 'recovery' $* || exit
endif

######## OPTIONS

install: install.components

%.install: ;

clean: rclean clean.components

depend: depend.components

depend-clean: depend-clean.components

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
