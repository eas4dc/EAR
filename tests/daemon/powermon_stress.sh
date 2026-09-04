#!/bin/bash

if [ $# -lt 5 ]; then
	echo "Usage: $0 <prefix> <ear-etc> <ear-tmp> <# steps> <nas-bin>"
	exit 1
fi

ear_conf=$2/ear/ear.conf

if [ ! -f $ear_conf ]; then
	echo "Error: $ear_conf not found."
	exit 1
fi

set -e
set -x

sed -i -e "s/\(NodeUseDB=\)\([01]\)/\11/" \
	-e "s/\(NodeUseEARDBD=\)\([01]\)/\11/" \
	-e "s/\(NodeDaemonPowermonFreq=\)\([[:digit:]]\+\)/\110/"	\
	-e "s/\(EARDReportPlugins=\)\([[:alnum:]_]\+\.so\)/\1log.so/" \
	-e "s/\(NodeDaemonMinPstate=\)\([[:digit:]]\+\)/\10/" \
	-e "s/\(energy_plugin=\)\([[:alnum:]_]\+\.so\)/\1energy_dummy.so/" 	$ear_conf

echo "Island=0 Nodes=$(hostname) EARDBD_IP=$(hostname)" >> $ear_conf

export EAR_ETC=$2
$1/sbin/eard 3 &> eard.out &
eard_pid=$!

sleep 2

# Setting the environment for running the erun

export EAR_TMP=$3
export EAR_INSTALL_PATH=$1

export SLURM_JOB_ID=1
for (( i=1 ; i<=${4} ; i++ )); do
	$EAR_INSTALL_PATH/bin/erun --program="$5"
done
$EAR_INSTALL_PATH/bin/erun --clean

if [[ ! $(ps | grep $eard_pid) ]]; then
	echo "The eard [$eard_pid] is not running... Check the eard.out file."
	cat eard.out
	exit 1
fi

echo "eard [$eard_pid] is running!"

echo "Checking whether the number of apps reported matches ${4}..."
if [[ $(cat $EAR_TMP/eard.*.apps.txt | wc -l) -eq ${4} ]]; then
	echo "Match!"
	exit 0
else
	echo "$(cat $EAR_TMP/eard.*.apps.txt | wc -l) does not match with ${4}"
	exit 1
fi
