CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC

######## FILES

ener_OBJS = \
    energy_nm.o \
    energy_rapl.o \
    energy_dcmi.o \
    energy_sd650.o \
    energy_inm_power.o \
    energy_SD650N_V2.o \
    energy_dummy.o

ifdef FREEIPMI_BASE
ener_OBJS +=\
		energy_inm_power_freeipmi.o
endif

ener_BINS = \
    energy_nm.so \
    energy_rapl.so \
    energy_dcmi.so \
    energy_sd650.so \
    energy_inm_power.so \
    energy_SD650N_V2.so \
    energy_dummy.so

ifdef FREEIPMI_BASE
ener_BINS += \
		energy_inm_power_freeipmi.so
endif

ener_PATH = $(DESTDIR)/lib/plugins/energy
ener_PERM = 0775
ener_DEPS = \
    $(SRCDIR)/common/libcommon.a
rapl_DEPS = \
    $(SRCDIR)/metrics/common/omsr.o \
    $(SRCDIR)/metrics/energy/cpu/intel63.o

######## RULES

all: $(ener_BINS)

energy_inm_power_freeipmi.o:energy_inm_power_freeipmi.c
	$(CC) $(CC_FLAGS) $(CFLAGS) $(FREEIPMI_CFLAGS) -c $<

energy_inm_power_freeipmi.so:energy_inm_power_freeipmi.o
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(FREEIPMI_LDFLAGS) $(ener_DEPS)

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
