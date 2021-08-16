#!/bin/bash
export EAR_TMP=/var/run/ear
export SLURM_PATH=/hpc/opt/slurm/bin/
export PATH=$SLURM_PATH:$PATH
echo "###############################################################"																					>> $EAR_TMP/ear_power_save.log
echo "EAR powercap suspend action: current_power $1 current_limit $2 total_idle_nodes $3 total_idle_power $4"		>> $EAR_TMP/ear_power_save.log
echo "###############################################################"																					>> $EAR_TMP/ear_power_save.log
if [ $3 -eq 0 ]; then 
	exit
fi

echo "`date` Suspend invoked " >> $EAR_TMP/ear_power_save.log
sinfo -r --states=idle -N -h > $EAR_TMP/idle.txt
awk '{print $1 "\t"}' $EAR_TMP/idle.txt > $EAR_TMP/nodelist.txt
export HOSTLIST="$(echo $(cat $EAR_TMP/nodelist.txt))"

rm -f $EAR_TMP/ear_stopped_nodes.txt
for i in ${HOSTLIST}
do
		echo ${i} >> $EAR_TMP/ear_stopped_nodes.txt
		echo "Node ${i} set to DRAIN " >> $EAR_TMP/ear_power_save.log
    scontrol update NodeName=${i} state=drain reason="EAR powercap"
done
rm -f $EAR_TMP/idle.txt
rm -f $EAR_TMP/nodelist.txt
