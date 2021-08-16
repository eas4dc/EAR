CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC -Wall -g

######## FILES

driv_OBJS = linux_cpufreq.o

######## RULES

all: $(driv_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
