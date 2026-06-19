CC = gcc
CFLAGS = -Wall -Werror -I ../../../ -fPIC --shared

plug_BINS = \
    deps1.so \
    deps2.so \
    deps3.so \
    dummy.so \
    keyboard.so \
    keyboard_cpufreq.so \
    sockets_receiver.so \
    sockets_sender.so

all: $(plug_BINS)

%.so: %.c
	$(CC) $(CFLAGS) -o $@ $^

kernel_cuda.so: kernel_cuda.c kernel_cuda_ptx.cu
	$(CUDA_BASE)/bin/nvcc $(CFLAGS) --compiler-options '-fPIC'  -o $@ $^

kernel_cl.so: kernel_cl.c
	$(CC) $(CFLAGS) $(OPENCL_CFLAGS) -D CL_TARGET_OPENCL_VERSION=100 -o $@ $^ $(OPENCL_LDFLAGS)
