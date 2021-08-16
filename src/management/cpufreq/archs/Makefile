CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC -Wall

######## OPTIONS

ifeq ($(FEAT_AMD_OSCPUFREQ), 1)
DEFS += -DAMD_OSCPUFREQ=1
endif


######## FILES

arch_OBJS = \
    amd17.o \
    default.o

######## RULES

all: $(arch_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) $(DEFS) -c $<

######## OPTIONS

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
