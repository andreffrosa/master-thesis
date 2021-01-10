#!/bin/bash

MIN_ARGS=2
if [ "$#" -lt $(($MIN_ARGS)) ]
then
   echo "usage: ./test_broadcast_local.sh <n_pis> <duration_s>"
   exit
fi

PIS=$1
DURATION=$2

TESTS=(
#"broadcast/flooding"
#"broadcast/gossip1"
#"broadcast/gossip1_horizon"
#"broadcast/gossip2;discovery/PeriodicHelloDiscovery"
#"broadcast/gossip3"
#"broadcast/rapid;discovery/PeriodicHelloDiscovery"
#"broadcast/enhanced_rapid;discovery/PeriodicHelloDiscovery"
#"broadcast/counting"
#"broadcast/counting_parents"
#"broadcast/hop_count_aided"
#"broadcast/naba1;discovery/PeriodicHelloDiscovery"
#"broadcast/naba2;discovery/PeriodicHelloDiscovery"
#"broadcast/naba3;discovery/PeriodicJointDiscovery"
#"broadcast/naba4;discovery/PeriodicJointDiscovery"
#"broadcast/sba;discovery/PeriodicJointDiscovery"
"broadcast/mpr;discovery/OLSRDiscovery"
#"broadcast/ahbp;discovery/PeriodicJointDiscovery"
"broadcast/lenwb;discovery/LENWBDiscovery"
#"broadcast/dynamic_probability"

#"broadcast/rad_extension"
#"broadcast/hop_count_rad_extension"
)

i=1
for t in ${TESTS[@]}; do
    ARR=( $(echo $t | sed 's/;/ /g') )

    BCAST=${ARR[0]}
    DISC=${ARR[1]}

    D=""
    if [ "$DISC" = "" ] ; then
        D=""
    else
        D=" -d ../experiments/configs/"$DISC".conf"
    fi

    EXP=$BCAST
    i=$((i+1))
    EXE="../bin/broadcast_test -b ../experiments/configs/$BCAST.conf"$D" -a ../experiments/configs/broadcast_app.conf -o ../topologies/exp1/"
    sudo ./test_algortihm_local.sh $PIS $DURATION $EXP "$EXE"
done
