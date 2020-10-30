######## RULES

.PHONY: check

all: all.components

install: install.components;

%.install: ;

clean: clean.components;

######## OPTIONS

check:
	@ $(MAKE) -C check check || exit

depend: depend.components

depend-clean: depend-clean.components

######## SUBFOLDERS

%.components:
	@ $(MAKE) -C 'check' $* || exit
	@ $(MAKE) -C 'functionals' $* || exit
