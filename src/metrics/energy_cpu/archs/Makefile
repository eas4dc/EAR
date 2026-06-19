CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

temp_OBJS = \
    msr.o \
	cpu_util.o \
    eard.o \
	hwmon.o \
    linux_powercap.o \
	perf.o \
    acpi_power.o \
    grace_cpu_power.o \
	dummy.o 

######## RULES

all: $(temp_OBJS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

######## OPTIONS

install: ;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
