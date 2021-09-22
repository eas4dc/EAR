CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

ener_OBJS = \
    energy_nm.o \
    energy_rapl.o \
    energy_dcmi.o \
    energy_sd650.o \
		energy_inm_power.o
ener_BINS = \
    energy_nm.so \
    energy_rapl.so \
    energy_dcmi.so \
    energy_sd650.so \
		energy_inm_power.so

ener_PATH = $(DESTDIR)/lib/plugins/energy
ener_PERM = 0775
ener_DEPS = \
    $(SRCDIR)/common/libcommon.a
rapl_DEPS = \
    $(SRCDIR)/metrics/common/omsr.o \
    $(SRCDIR)/metrics/energy/cpu/intel63.o

######## RULES

all: $(ener_BINS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

energy_rapl.so: energy_rapl.o $(rapl_DEPS) $(ener_DEPS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(rapl_DEPS) $(ener_DEPS) -lm 

%.so: %.o $(ener_DEPS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(ener_DEPS) 

######## OPTIONS

install: ener.ginstall;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
