CFLAGS = $(CC_FLAGS) -I$(SRCDIR) -fPIC -D_GNU_SOURCE


######## FILES

ener_OBJS = \
    energy_nm.o \
    energy_rapl.o \
    energy_dcmi.o \
    energy_sd650.o \
    energy_inm_power.o \
    energy_dummy.o \
    energy_cpu_gpu.o \
    energy_sd650_power.o \
    energy_eard_nm.o \
		energy_cray_pm.o

ifdef FREEIPMI_BASE
ener_OBJS += energy_inm_power_freeipmi.o
endif

ifdef REDFISH_BASE
ener_OBJS += energy_redfish.o
endif

ifeq ($(SYSTEM_TYPE), 3)
# Define and uncomment these two constants with the EAR installation path if you want to enable the eard_nm energy plugin
#EAR_OFFICIAL_NM_INSTALL_PATH=
#EAR_OFFICIAL_TMP=
ener_OBJS += energy_eard_nm.o
endif

ener_BINS = \
    energy_nm.so \
    energy_rapl.so \
    energy_dcmi.so \
    energy_sd650.so \
    energy_inm_power.so \
    energy_cpu_gpu.so \
    energy_dummy.so \
    energy_sd650_power.so  \
		energy_cray_pm.so

ifeq ($(SYSTEM_TYPE), 3)
    ener_BINS += energy_eard_nm.so
endif

ifdef FREEIPMI_BASE
ener_BINS += \
		energy_inm_power_freeipmi.so
endif

ifdef REDFISH_BASE
ener_BINS += \
		energy_redfish.so
endif

ener_PATH = $(DESTDIR)/lib/plugins/energy
ener_PERM = 0775
ener_DEPS = \
    $(SRCDIR)/common/libcommon.a
rapl_DEPS = \
    $(SRCDIR)/metrics/common/omsr.o \
    $(SRCDIR)/metrics/energy/cpu/intel63.o

cray_DEPS = $(SRCDIR)/metrics/common/cray_pm_counters.o

redfish_DEPS = \
	$(SRCDIR)/metrics/common/redfish.o

######## RULES

all: $(ener_BINS)

energy_inm_power_freeipmi.o:energy_inm_power_freeipmi.c
	$(CC) $(CC_FLAGS) $(CFLAGS) $(FREEIPMI_CFLAGS) -c $<

energy_inm_power_freeipmi.so:energy_inm_power_freeipmi.o
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(FREEIPMI_LDFLAGS) $(ener_DEPS)

energy_redfish.so: energy_redfish.o $(ener_DEPS) $(redfish_DEPS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(redfish_DEPS) $(ener_DEPS)

energy_cpu_gpu.so:energy_cpu_gpu.o $(ener_DEPS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(ener_DEPS) -lm 

energy_cray_pm.so:energy_cray_pm.o $(ener_DEPS) $(cray_DEPS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(ener_DEPS) $(cray_DEPS) -lm

%.o: %.c
	$(CC) $(CFLAGS) -pthread -c $< 

energy_rapl.so: energy_rapl.o $(rapl_DEPS) $(ener_DEPS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@ $(rapl_DEPS) $(ener_DEPS) -lm 

%.so: %.o $(ener_DEPS)
	$(CC) $(CC_FLAGS) $(CFLAGS) -pthread -shared $< -o $@ $(ener_DEPS) 

energy_eard_nm.so:energy_eard_nm.c
	$(CC) $(CC_FLAGS) -DEAR_NM_TMP=\"$(EAR_OFFICIAL_TMP)\" -I$(EAR_OFFICIAL_NM_INSTALL_PATH)/include $(CFLAGS) -pthread -shared $< -o $@ $(ener_DEPS) -L$(EAR_OFFICIAL_NM_INSTALL_PATH)/lib -lear_api

######## OPTIONS

install: ener.ginstall;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
