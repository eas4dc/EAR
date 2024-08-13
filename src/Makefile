######## FILES

COMPONENTS = \
     common \
     report \
     metrics \
     management\
     daemon \
     library \
     global_manager \
     tools \
     slurm_plugin \
     pbs_hook \
     database_cache \
     commands \
     data_center_monitor

ifdef FEAT_ONLY_EAR_CORE
COMPONENT_DISABLE += \
    commands \
    tools \
    database_cache
endif


ifeq ($(FEAT_WF_SUPPORT), 1)  
CC_FLAGS   += -DWF_SUPPORT=1
MPI_CFLAGS += -DWF_SUPPORT=1
else
CC_FLAGS   += -DWF_SUPPORT=0
MPI_CFLAGS += -DWF_SUPPORT=0

COMPONENT_DISABLE += \
     commands
endif

######## RULES

.PHONY:

all: all.components

post:
	@ $(MAKE) -C 'metrics'    $@ || exit
	@ $(MAKE) -C 'management' $@ || exit
	@ $(MAKE) -C 'library' $@ || exit

install: install.components

clean: clean.components

######## ALIASES

etc.install: etc.install.components;

devel.install: devel.install.components;

eard.install: daemon.install;

eardbd.install: database_cache.install;

earl.install: library.install;

eargmd.install: global_manager.install;

earplug.install: slurm_plugin.install;

pbshook.install: pbs_hook.install;

%.install:
	@ $(MAKE) -C $* install || exit

%.build:
	@ $(MAKE) -C $* clean || exit
	@ $(MAKE) -C $* all || exit

######## SUBFOLDERS

%.components:
	@ for i in $(COMPONENTS); do                            \
		if ([ -z "${COMPONENT_ENABLE}"  ] && printf %s\\n "${COMPONENT_DISABLE}" | grep -qFv "$$i") || \
           ([ -z "${COMPONENT_DISABLE}" ] && printf %s\\n "${COMPONENT_ENABLE}"  | grep -qF  "$$i"); then \
        	echo "====================================";    \
        	echo "= Compiling $$i";                         \
        	echo "====================================";    \
        	$(MAKE) -C $${i} $* || exit;                    \
        fi                                                  \
    done

######## DEPENDENCIES

depend: depend.components

depend-clean: depend-clean.components
