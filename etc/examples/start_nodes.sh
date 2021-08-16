#!/bin/bash
export EAR_TMP=/var/run/ear
export SLURM_PATH=/hpc/opt/slurm/bin/
export PATH=$SLURM_PATH:$PATH

echo "###############################################################" 																				>> $EAR_TMP/ear_power_save.log
echo "EAR powercap resume action: current_power $1 current_limit $2 total_idle_nodes $3 total_idle_power $4" 	>> $EAR_TMP/ear_power_save.log
echo "###############################################################" 																				>> $EAR_TMP/ear_power_save.log
echo "`date` Resume invoked " >> $EAR_TMP/ear_power_save.log

export HOSTLIST="$(echo $(cat $EAR_TMP/ear_stopped_nodes.txt))"

for i in ${HOSTLIST}
do
    echo "Setting idle node=${i}" >> $EAR_TMP/ear_power_save.log
    scontrol update NodeName=${i} state=idle
done
rm -f $EAR_TMP/ear_stopped_nodes.txt
