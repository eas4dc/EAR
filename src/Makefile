######## FILES

COMPONENTS = common 
ifdef DB_BASE
	COMPONENTS += database_cache
endif
ifdef PAPI_BASE
	COMPONENTS += \
                  metrics \
                  daemon \
                  commands \
                  tests \
                  library
endif
ifdef GSL_BASE
	COMPONENTS += tools
endif
ifdef DB_BASE
	COMPONENTS += global_manager
endif
COMPONENTS += slurm_plugin

######## RULES

.PHONY: check

all: all.components

check:
	@ $(MAKE) -C tests check

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

%.install:
	@ $(MAKE) -C $* install || exit

%.build:
	@ $(MAKE) -C $* clean || exit
	@ $(MAKE) -C $* all || exit

######## SUBFOLDERS

%.components:
	@ for i in $(COMPONENTS); do                     \
		echo "===================================="; \
		echo "= Compiling $$i";                      \
		echo "===================================="; \
		$(MAKE) -C $${i} $* || exit;                 \
	done

######## DEPENDENCIES

depend: depend.components

depend-clean: depend-clean.components
