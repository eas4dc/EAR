CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) -O2 
LDFLAGS = $(DB_LDFLAGS) -lm

######## INSTALLATION

eard_BINS = eardbd
eard_PATH = $(DESTDIR)/sbin
eard_PERM = 0770
oths_BINS = eardbd_api.o

######## FILES

eard_LIBS = \
	$(SRCDIR)/management/libmgt.a \
	$(SRCDIR)/metrics/libmetrics.a \
	$(SRCDIR)/common/libcommon.a \

eard_SRCS = eardbd.c \
    eardbd_body.c \
    eardbd_sync.c \
    eardbd_signals.c \
    eardbd_storage.c

eard_HDRS = eardbd.h \
    eardbd_body.h \
    eardbd_sync.h \
    eardbd_signals.h \
    eardbd_storage.h

######## RULES

all: $(oths_BINS) $(eard_BINS)

eardbd_api.o: eardbd_api.c eardbd_api.h $(eard_LIBS)
	$(CC) $(CFLAGS) -c $<

eardbd: $(eard_SRCS) $(eard_HDRS) $(eard_LIBS)
	$(CC) $(CFLAGS) $(DB_CFLAGS) -o $@ $(eard_SRCS) $(eard_LIBS) $(LDFLAGS)

######## OPTIONS

install: eard.ginstall;

%.install: ;

clean: eard.gclean rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
