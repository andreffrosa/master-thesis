#!/bin/bash

MIN_ARGS=2
if [ "$#" -lt $(($MIN_ARGS)) ]
then
   echo "usage: ./test_algorithm_local.sh <n_pis> <duration_s>"
   exit
fi

PIS=$1
DURATION=$2

EXP="exp1"

#EXE="../bin/broadcast_test"
EXE="../bin/discovery_test"

DISCOVERY="-d ../experiments/configs/discovery.conf"
#BROADCAST="-b ../experiments/configs/broadcast.conf"
BROADCAST=""
#APP="-a ../experiments/configs/broadcast_app.conf"
APP="-a ../experiments/configs/discovery_app.conf"
OVERLAY="-o ../topologies/exp1/"




# Launch virtual network
echo "Launching virtual network with $PIS nodes ..."
sudo ./launch_virtual_network.sh $PIS > /dev/null 2>&1

echo ""

# Ensure virtual nodes are connected
for PI in $(seq 1 $PIS); do
    HOSTNAME="raspi-0"$PI
    INTERFACE="wlan"$(($PI-1))

    echo "Connecting virtual node $PI (HOSTNAME=$HOSTNAME INTERFACE=$INTERFACE) ..."

    sudo ../bin/connect_test -i $INTERFACE -h $HOSTNAME > /dev/null 2>&1
    RES=$?
    while [ "$RES" -ne "0" ]; do
        sudo ../bin/connect_test -i $INTERFACE -h $HOSTNAME > /dev/null 2>&1
        RES=$?
        sleep 2
    done
done

sleep 5

echo ""

# Launch test
for PI in $(seq 1 $PIS); do
    HOSTNAME="raspi-0"$PI
    INTERFACE="wlan"$(($PI-1))

    OUT="../experiments/output/$EXP-$HOSTNAME.log"

    CMD="sudo $EXE $DISCOVERY $BROADCAST $APP $OVERLAY -i $INTERFACE -h $HOSTNAME"

    echo "Launching test on virtual node $PI ($CMD) ..."

    sudo rm $OUT

    nohup $CMD > $OUT 2>&1 &
done

echo ""

echo "0 s"
for i in $(seq 1 "$(($DURATION / 10))"); do
    sleep 10
    echo "$(($i*10)) s"
done

echo ""

# Kill test
echo "Killing Test ..."

#PIDS=( $(ps ax | grep "sudo $EXE" | grep -v "grep" | awk '{print $1}') )
#while [ "${#PIDS[@]}" -gt 0 ]; do
#    for PID in "${PIDS[@]}"; do
#        CMD="sudo kill -9 $PID"
#        echo $CMD
#        $($CMD)
#    done
#
#    sleep 1
#
#    PIDS=( $(ps ax | grep "sudo $EXE" | grep -v "grep" | awk '{print $1}') )
#done

sudo kill -9 $(ps ax | grep "$EXE" | grep -v "grep" | awk '{print $1}' )
ps ax | grep "$EXE" | grep -v "grep" | awk '{print $1}'

# Kill virtual network
#sudo ./kill_virtual_network.sh
