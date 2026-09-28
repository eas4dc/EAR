#!/bin/bash

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <prefix> <ear-etc> <report-plugins>" >&2
    exit 2
fi

prefix=$1
ear_etc=$2
report_plugins=$3
ear_conf="$ear_etc/ear/ear.conf"

if [ ! -f "$ear_conf" ]; then
    echo "Error: $ear_conf not found." >&2
    exit 1
fi

set -e
set -x
trap 'status=$?; echo "ERROR: deploy_eardbd.sh failed at line $LINENO (status $status): $BASH_COMMAND" >&2; exit "$status"' ERR

node=$(hostname)
sed -i \
    -e "s|^EARDBDReportPlugins=.*|EARDBDReportPlugins=$report_plugins|" \
    -e 's|^TmpDir=.*|TmpDir=/tmp/ear|' \
    -e 's|^DBDaemonInsertionTime=.*|DBDaemonInsertionTime=1|' \
    -e "s|^EARDBD_IP=.*|EARDBD_IP=$node|" \
    -e '/^Island=/d' \
    "$ear_conf"
printf 'Island=0 Nodes=%s EARDBD_IP=%s\n' "$node" "$node" >> "$ear_conf"

mkdir -p /tmp/ear
export EAR_ETC=$ear_etc
"$prefix/sbin/eardbd" > eardbd.out 2>&1 &
echo "$!" > eardbd.pid
