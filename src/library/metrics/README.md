# EAR Library Metrics module

This module contains all components related to getting metric data inside EARL.
It is used by the Library's main loop on **states** to compute the application signature and **policies** to work with concrete metrics APIs, defined at `src/metrics`.

## DCGMI

A module which handles DCGMI API integration.
The main **metrics** module interacts with it.
You can view [here](https://gitlab.bsc.es/ear_team/ear_private/-/issues/144/designs/dcgmi.png) the sequence diagram.

It currently creates a periodic thread to collect all basic [NVIDIA DCGM profiling metrics](https://docs.nvidia.com/datacenter/dcgm/latest/user-guide/feature-overview.html#profiling-metrics) because not all events can be requested together.
Therefore based on the GPU model, different event sets are created and multiplexed for reading DCGM event metrics.
Current supported GPU architectures are **Turing**, **Hopper** and **Ampere**.

### Testing

Target `post` from the source root directory, i.e., `make post` and `dcgmi_lib_test` tool will be ready to use.
The binary is located at `src/library/metrics`.
You need to export `libear.so` and `libearld.so` libraries paths to run this test.
You may also need to enable EARL's DCGM feature.

```
export EAR_GPU_DCGMI_ENABLED=1
export LD_LIBRARY_PATH=/path/to/libearld_so:/path/to/libear_so:$LD_LIBRARY_PATH
./dcgmi_lib_test
```

On the same system where you are going to test, run a GPU workload.
As this test targets systems which have NVIDIA DCGM installed, you can run [CUDA Test Generator (dcgmproftester)](https://docs.nvidia.com/datacenter/dcgm/latest/user-guide/feature-overview.html#cuda-test-generator-dcgmproftester).

```
dcgmproftester11 --no-dcgm-validation -t 1004,1005
```

## Fine grained metrics

A module which handles the collection of specific metrics at a faster frequency than the main **metrics** module.
It is useful when you need more precision in the time window of any metric.
