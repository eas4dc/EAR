#!/bin/bash

# Export required modules here:

export EAR_GPU_DCGMI_ENABLED=1
export LD_LIBRARY_PATH=../../:../../loader:$LD_LIBRARY_PATH
../dcgmi_lib_test
