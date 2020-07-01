######## FILES

modl_OBJS = models.o \
       basic_model.o \
			avx512_model.o

modl_BINS = basic_model.so avx512_model.so
modl_PATH = $(DESTDIR)/lib/plugins/models
modl_PERM = 0775

########

all: $(modl_OBJS) $(modl_BINS)

%.o: %.c %.h
	$(CC) -I${SRCDIR} $(CC_FLAGS) $(CFLAGS) -c $<

%.so: %.o
	$(CC) $(CC_FLAGS) $(CFLAGS) -shared $< -o $@  


######## OPTIONS 

install: modl.ginstall;

clean: rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
