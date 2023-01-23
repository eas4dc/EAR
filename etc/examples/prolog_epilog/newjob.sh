#!/bin/bash
if [ $SLURM_LOCALID -eq 0 ]; then
$EAR_INSTALL_PATH/ejob 50001 newjob
fi

