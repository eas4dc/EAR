#!/bin/bash

SLURM_COMP_VERBOSE=3 EAR_DEFAULT=off EAR_TMP=/tmp EAR_ETC=/tmp EAR_INSTALL_PATH=/tmp ./erun --program="$1 $2" &
sleep 1
SLURM_COMP_VERBOSE=3 EAR_DEFAULT=off EAR_TMP=/tmp EAR_ETC=/tmp EAR_INSTALL_PATH=/tmp ./erun --program="$1 $2"
