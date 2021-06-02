#!/bin/bash

MIN_ARGS=4
if [ "$#" -lt $(($MIN_ARGS)) ]
then
   echo "usage: ./test_algorithm_local.sh <n_pis> <duration_s> <exp> <exe>"
   exit
fi

PIS=$1
DURATION=$2
EXP=$3
EXE=$4
L_DIR=$5

echo $PIS $DURATION $EXP $EXE

#exit

#EXP="exp1"

#EXE="../bin/broadcast_test"
#EXE="../bin/discovery_test"

#DISCOVERY="-d ../experiments/configs/discovery.conf"
#BROADCAST="-b ../experiments/configs/broadcast.conf"
#BROADCAST=""
#APP="-a ../experiments/configs/broadcast_app.conf"
#APP="-a ../experiments/configs/discovery_app.conf"
#OVERLAY="-o ../topologies/exp1/"




# Launch virtual network
#echo "Launching virtual network with $PIS nodes ..."
#sudo ./launch_virtual_network.sh $PIS > /dev/null 2>&1

echo ""

# Ensure virtual nodes are connected
#for PI in $(seq 1 $PIS); do
#    HOSTNAME="raspi-0"$PI
#    INTERFACE="wlan"$(($PI-1))

#    echo "Connecting virtual node $PI (HOSTNAME=$HOSTNAME INTERFACE=$INTERFACE) ..."

#    sudo ../bin/connect_test -i $INTERFACE -h $HOSTNAME > /dev/null 2>&1
#    RES=$?
#    while [ "$RES" -ne "0" ]; do
#        sudo ../bin/connect_test -i $INTERFACE -h $HOSTNAME > /dev/null 2>&1
#        RES=$?
#        sleep 2
#    done
#done

#sleep 5

echo ""

# Launch test
for PI in $(seq 1 $PIS); do
    HOSTNAME="raspi-0"$PI
    INTERFACE="wlan"$(($PI-1))

    OUT="../experiments/output/$EXP-$HOSTNAME.log"

    ARR=( $(echo $EXP | sed 's/\// /g') )
    #d1=${ARR[0]}
    d2=${ARR[1]}
    L="-l $L_DIR/$HOSTNAME/$d2-run1/"
    mkdir "$L_DIR/$HOSTNAME/" > /dev/null 2>&1

    CMD="sudo $EXE -i $INTERFACE -h $HOSTNAME $L"

    echo "Launching node $PI ($CMD) ..."

    sudo rm $OUT > /dev/null 2>&1

    nohup unbuffer $CMD > $OUT 2>&1 &
    #nohup $SHELL -c "$CMD > $OUT 2>&1 | unbuffer -p echo" > $OUT 2>&1 &
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

#sudo kill -9 $(ps ax | grep "$EXE" | grep -v "grep" | awk '{print $1}' ) > /dev/null 2>&1
#ps ax | grep "$EXE" | grep -v "grep" | awk '{print $1}'

./kill_test.sh "$EXE"

# Kill virtual network
#sudo ./kill_virtual_network.sh
