CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) -O2  $(CONSTANTS)
LDFLAGS = -pthread -lm -rdynamic -ldl

######## INSTALLATION

eardbd_BINS = eardbd
eardbd_PATH = $(DESTDIR)/sbin
eardbd_PERM = 0770
othsbd_BINS = eardbd_api.o

######## FILES

eardbd_LIBS = \
	$(SRCDIR)/report/report.o \
	$(SRCDIR)/management/libmgt.a \
	$(SRCDIR)/metrics/libmetrics.a \
	$(SRCDIR)/common/libcommon.a \

eardbd_SRCS = eardbd.c \
    eardbd_body.c \
    eardbd_sync.c \
    eardbd_signals.c \
    eardbd_storage.c

eardbd_HDRS = eardbd.h \
    eardbd_body.h \
    eardbd_sync.h \
    eardbd_signals.h \
    eardbd_storage.h

######## RULES

all: $(oths_BINS) $(eardbd_BINS)

eardbd_api.o: eardbd_api.c eardbd_api.h $(eardbd_LIBS)
	$(CC) $(CFLAGS) -c $<

eardbd: $(eardbd_SRCS) $(eardbd_HDRS) $(eardbd_LIBS)
	$(CC) $(CFLAGS) -o $@ $(eardbd_SRCS) $(eardbd_LIBS) $(LDFLAGS)

######## OPTIONS

install: eardbd.ginstall;

%.install: ;

clean: eardbd.gclean rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
