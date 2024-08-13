# Examples
# CFLAGS_AVX512 = $(CFLAGS) -msse4.1  -msse4.2 -msse3 -mavx512dq -mavx512f -mavx -mcrc32
# CFLAGS_AVX2   = $(CFLAGS) -msse4.1  -msse4.2 -msse3 -mavx -mcrc32

CFLAGS        = -fPIC -O3 -I$(SRCDIR)
CFLAGS_AVX2   = $(CFLAGS) -mavx2
CFLAGS_AVX512 = $(CFLAGS) -march=skylake-avx512

######## FILES

OBJS = dynais.o
DEPS = dynais.o

ifeq ($(ARCH), X86)
OBJS += avx2/dynais.o \
        avx2/dynais_core_0.o \
        avx2/dynais_core_n.o
DEPS += avx2/dynais.o \
        avx2/dynais_core.o
ifeq ($(FEAT_AVX512), 1)
OBJS += avx512/dynais.o \
        avx512/dynais_core_0.o \
        avx512/dynais_core_n.o
DEPS += avx512/dynais.o \
        avx512/dynais_core.o
DEFI  = -DFEAT_AVX512=1
endif
endif

######## RULES

all: dynais.a

avx512/dynais_core.o: avx512/dynais_core.c avx512/dynais_core.h
	$(CC) $(CC_FLAGS) $(CFLAGS_AVX512) -DDYN_CORE_N -o avx512/dynais_core_n.o -c $<
	$(CC) $(CC_FLAGS) $(CFLAGS_AVX512) -DDYN_CORE_0 -o avx512/dynais_core_0.o -c $<

avx2/dynais_core.o: avx2/dynais_core.c avx2/dynais_core.h
	$(CC) $(CC_FLAGS) $(CFLAGS_AVX2) -DDYN_CORE_N -o avx2/dynais_core_n.o -c $<
	$(CC) $(CC_FLAGS) $(CFLAGS_AVX2) -DDYN_CORE_0 -o avx2/dynais_core_0.o -c $<

avx512/dynais.o: avx512/dynais.c avx512/dynais.h
	$(CC) $(CC_FLAGS) $(CFLAGS_AVX512) -o avx512/dynais.o -c $<

avx2/dynais.o: avx2/dynais.c avx2/dynais.h
	$(CC) $(CC_FLAGS) $(CFLAGS_AVX2) -o avx2/dynais.o -c $<

dynais.o: dynais.c dynais.h
	$(CC) $(CC_FLAGS) $(CFLAGS) -march=native $(DEFI) -o dynais.o -c $<

dynais.a: $(DEPS)
	$(AR) rvs $@ $(OBJS)

######## OPTIONS

install: ;

clean: rclean
	rm -f ./avx2/*.o ./avx512/*.o

######## EXPORTS

include $(SRCDIR)/Makefile.depend
include $(SRCDIR)/Makefile.extra
