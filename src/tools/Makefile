CFLAGS  = $(CC_FLAGS) -I$(SRCDIR) $(GSL_CFLAGS)
LDFLAGS = $(GSL_LDFLAGS) -lm
CCC     = $(CC) $(CFLAGS)

######## INSTALLATION

tool_BINS = \
    coeffs_quality \
    coeffs_compute \
    coeffs_compare \
    coeffs_jobproj \
    coeffs_show \
    coeffs_default \
    coeffs_null \
    ear_events \
    get_active_nodes \
    learning_show \
    learning_validate \
    learning_delete
tool_PATH = $(DESTDIR)/bin/tools
tool_PERM = 0770

######## FILES

comm_OBJS = $(ROOTDIR)/src/common/libcommon.a -lm

######## RULES

all: $(tool_BINS)

coeffs_compute: coeffs_compute.c $(comm_OBJS)
	$(CCC) -g -o $@ $< $(LDFLAGS) $(comm_OBJS) $(DB_LDFLAGS)

coeffs_jobproj: coeffs_jobproj.c $(comm_OBJS)
	$(CCC) -o $@ $< $(comm_OBJS) $(DB_LDFLAGS) -ldl

coeffs_quality: coeffs_quality.c $(comm_OBJS)
	$(CCC) -o $@ $< $(comm_OBJS) $(DB_LDFLAGS) -ldl

coeffs_show: coeffs_show.c $(comm_OBJS)
	$(CCC) -I$(SRCDIR) -o $@ $< $(comm_OBJS)

coeffs_null: coeffs_null.c $(comm_OBJS)
	$(CCC) -I$(SRCDIR) -o $@ $< $(comm_OBJS)

coeffs_default: coeffs_default.c $(comm_OBJS)
	$(CCC) -I$(SRCDIR) -o $@ $< $(comm_OBJS)

coeffs_compare: coeffs_compare.c $(comm_OBJS)
	$(CCC) -I$(SRCDIR) -o $@ $< $(comm_OBJS)

ear_events: ear_events.c $(comm_OBJS)
	$(CCC) -g -o $@ $< $(comm_OBJS) $(DB_LDFLAGS)

get_active_nodes: get_active_nodes.c $(comm_OBJS)
	$(CCC) -g -o $@ $< $(comm_OBJS) $(DB_LDFLAGS)

learning_show: learning_show.c $(comm_OBJS)
	$(CCC) -o $@ $< $(comm_OBJS) $(DB_LDFLAGS)

learning_delete: learning_delete.c $(comm_OBJS)
	$(CCC) -g -o $@ $< $(comm_OBJS) $(DB_LDFLAGS)

learning_validate: learning_validate.c $(comm_OBJS)
	$(CCC) -o $@ $< $(LDFLAGS) $(comm_OBJS) $(DB_LDFLAGS)

######## OPTIONS

install: tool.ginstall;

%.install: ;

clean: tool.gclean rclean;

######## DEPENDENCIES

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
