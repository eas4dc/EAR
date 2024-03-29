CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) $(GSL_CFLAGS) $(DB_CFLAGS)
LDFLAGS = $(GSL_LDFLAGS) -lm
CCC     = $(CC) $(CFLAGS)

######## INSTALLATION

tool_BINS = \
    coeffs_show \
    coeffs_default \
    coeffs_null \
    enode_control \
    ear_conf
ifdef DB_BASE
tool_BINS += \
    learning_show \
    get_periodic \
    metrics_show \
    insert_test \
    learning_delete
endif
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

metrics_show: metrics_show.c $(comm_OBJS)
	$(CCC) -o $@ $< $(comm_OBJS) $(DB_LDFLAGS) -lm

insert_test: insert_test.c $(comm_OBJS)
	$(CCC) -o $@ $< $(comm_OBJS) $(DB_LDFLAGS) -lm

get_periodic: get_periodic.c $(comm_OBJS)
	$(CCC) -o $@ $< $(comm_OBJS) $(DB_LDFLAGS) -lm

learning_delete: learning_delete.c $(comm_OBJS)
	$(CCC) -o $@ $< $(comm_OBJS) $(DB_LDFLAGS)

ear_conf: ear_conf.c $(comm_OBJS)
	$(CCC) -o $@ $< $(comm_OBJS) $(DB_LDFLAGS)

enode_control: ear_node_control.c $(comm_OBJS) 
	$(CCC) -o $@ $< $(comm_OBJS) $(DB_LDFLAGS)
######## OPTIONS

install: tool.ginstall;

%.install: ;

clean: tool.gclean rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
