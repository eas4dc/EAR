CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) $(GSL_CFLAGS)
LDFLAGS = $(GSL_LDFLAGS) -lm
CCC     = $(CC) $(CFLAGS)

######## INSTALLATION

tool_BINS = \
    coeffs_show \
    coeffs_default \
    coeffs_null \
    learning_show \
    learning_delete \
		ear_conf
ifdef GSL_BASE
tool_BINS += \
	coeffs_compute
endif
tool_PATH = $(DESTDIR)/bin/tools
tool_PERM = 0770

######## FILES

comm_OBJS = \
    $(SRCDIR)/common/libcommon.a

mang_OBJS = \
    $(SRCDIR)/management/libmgt.a \
	$(SRCDIR)/metrics/libmetrics.a

######## RULES

all: $(tool_BINS)

coeffs_compute: coeffs_compute.c $(mang_OBJS) $(comm_OBJS)
	$(CCC) -o $@ $< $(LDFLAGS) $(mang_OBJS) $(comm_OBJS) $(DB_LDFLAGS) -lpthread -ldl

coeffs_show: coeffs_show.c $(comm_OBJS)
	$(CCC) -I$(SRCDIR) -o $@ $< $(comm_OBJS)

coeffs_null: coeffs_null.c $(comm_OBJS)
	$(CCC) -I$(SRCDIR) -o $@ $< $(comm_OBJS)

coeffs_default: coeffs_default.c $(comm_OBJS)
	$(CCC) -I$(SRCDIR) -o $@ $< $(comm_OBJS)

learning_show: learning_show.c $(comm_OBJS)
	$(CCC) -o $@ $< $(comm_OBJS) $(DB_LDFLAGS) -lm

learning_delete: learning_delete.c $(comm_OBJS)
	$(CCC) -g -o $@ $< $(comm_OBJS) $(DB_LDFLAGS)

ear_conf: ear_conf.c $(comm_OBJS)
	$(CCC) -g -o $@ $< $(comm_OBJS) $(DB_LDFLAGS)

######## OPTIONS

install: tool.ginstall;

%.install: ;

clean: tool.gclean rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
