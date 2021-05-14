#!/bin/bash

MIN_ARGS=2
if [ "$#" -lt $(($MIN_ARGS)) ]
then
   echo "usage: ./test_ping_local.sh <n_pis> <duration_s>"
   exit
fi

PIS=$1
DURATION=$2

TESTS=(
#"routing/static"
#"routing/olsr;broadcast/mpr;discovery/OLSRDiscovery"
#"routing/aodv;broadcast/biflooding;discovery/PeriodicJointDiscovery"
#"routing/dsr;broadcast/biflooding;discovery/PeriodicJointDiscovery"
#"routing/zone;broadcast/mpr+biflooding;discovery/OLSRDiscovery"
#"routing/tora;broadcast/biflooding;discovery/PeriodicJointDiscovery"
"routing/batman;broadcast/batmanflooding;discovery/PassiveDiscovery2"
)

i=1
for t in ${TESTS[@]}; do
    ARR=( $(echo $t | sed 's/;/ /g') )

    ROUTING=${ARR[0]}
    BCAST=${ARR[1]}
    DISC=${ARR[2]}

    B=""
    if [ "$BCAST" = "" ] ; then
        B=""
    else
        B=" -b ../experiments/configs/"$BCAST".conf"
    fi

    D=""
    if [ "$DISC" = "" ] ; then
        D=""
    else
        D=" -d ../experiments/configs/"$DISC".conf"
    fi

    EXP=$ROUTING
    i=$((i+1))
    EXE="../bin/ping_test -r ../experiments/configs/$ROUTING.conf"$B""$D" -a ../experiments/configs/routing_app.conf -o ../topologies/exp1/"
    sudo ./test_algortihm_local.sh $PIS $DURATION $EXP "$EXE"
done
