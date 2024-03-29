######## FILES

COMPONENTS = \
    common \
    report \
    metrics \
    management\
    daemon \
    library \
    global_manager \
    commands \
	tools \
    slurm_plugin \

#ifdef DB_BASE
COMPONENTS += database_cache
#endif

######## RULES

.PHONY:

all: all.components

post:
	@ $(MAKE) -C 'metrics'    $@ || exit
	@ $(MAKE) -C 'management' $@ || exit

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
	@ for i in $(COMPONENTS); do                        \
		if ([ -z "${COMPONENT_ENABLE}"  ] && printf %s\\n "${COMPONENT_DISABLE}" | grep -qFv "$$i") || \
		   ([ -z "${COMPONENT_DISABLE}" ] && printf %s\\n "${COMPONENT_ENABLE}"  | grep -qF  "$$i"); then \
		echo "====================================";    \
		echo "= Compiling $$i";                         \
		echo "====================================";    \
		$(MAKE) -C $${i} $* || exit;                    \
		fi                                              \
	done

######## DEPENDENCIES

depend: depend.components

depend-clean: depend-clean.components
