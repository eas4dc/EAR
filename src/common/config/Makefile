######## FILES

# The list of libc_OBJS is in Makefile.files
dlog_BINS = defines.log
dlog_PATH = $(ETCDIR)/ear
dlog_PERM = 0660
dlog_FOLD = .

######## RULES

.PHONY: $(dlog_BINS)

all: $(dlog_BINS)

defines.log: 
	$(shell ./Makefile.script defines.log)

######## OPTIONS

install: dlog.finstall;

devel.install: ;

%.install: ;

clean: rclean;

######## SUBFOLDERS

%.components: ;

######## DEPENDENCIES

depend: ;

depend-clean: ;

include $(SRCDIR)/Makefile.extra
