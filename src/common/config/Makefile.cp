.PHONY: all

all: 
	$(shell ./Makefile.script)

######## OPTIONS

install: ;

devel.install: ;

%.install: ;

clean: rclean;

######## SUBFOLDERS

%.components: ;

######## DEPENDENCIES

depend: depend.components

depend-clean: depend-clean.components

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
