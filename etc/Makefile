######## FILES
                  
gen_FOLDERS = $(DESTDIR)/bin \
              $(DESTDIR)/bin/tests \
              $(DESTDIR)/bin/tools \
              $(DESTDIR)/include \
              $(DESTDIR)/lib \
              $(DESTDIR)/lib/plugins \
              $(DESTDIR)/lib/plugins/energy \
              $(DESTDIR)/lib/plugins/powercap \
              $(DESTDIR)/lib/plugins/models \
              $(DESTDIR)/lib/plugins/policies \
              $(DESTDIR)/lib/plugins/eard_policies \
              $(DESTDIR)/lib/plugins/tracer \
              $(DESTDIR)/lib/plugins/reports \
              $(DESTDIR)/sbin

etc_FOLDERS = $(ETCDIR) \
              $(ETCDIR)/ear \
              $(ETCDIR)/ear/coeffs \
              $(ETCDIR)/module \
              $(ETCDIR)/systemd \
              $(ETCDIR)/slurm 

conf_BINS = ear.conf.template ear.conf.full.template
conf_FOLD = conf
conf_PATH = $(ETCDIR)/ear
conf_PERM = 0660
modl_BINS = ear
modl_FOLD = module
modl_PATH = $(ETCDIR)/module
modl_PERM = 0666
serv_BINS = eard.service eardbd.service eargmd.service
serv_FOLD = systemd
serv_PATH = $(ETCDIR)/systemd
serv_PERM = 0660
slur_BINS = ear.plugstack.conf
slur_FOLD = slurm
slur_PATH = $(ETCDIR)/slurm
slur_PERM = 0660

######## OPTIONS

announce:
	@ echo "===================================="; \
	  echo "= Installing etc";                     \
	  echo "====================================";

install: announce
	@ mkdir -m 0775 -p $(DESTDIR) 
	@ mkdir -m 0775 -p $(gen_FOLDERS) 

etc.install: announce conf.finstall modl.finstall serv.finstall slur.finstall;
	@ mkdir -m 0775 -p $(etc_FOLDERS) 

devel.install: ;

scripts.install: install;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.extra
