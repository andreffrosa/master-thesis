#!/bin/bash

MIN_ARGS=2
if [ "$#" -lt $(($MIN_ARGS)) ]
then
   echo "usage: ./test_discovery_local.sh <n_pis> <duration_s>"
   exit
fi

PIS=$1
DURATION=$2

TESTS=(
#"discovery/NoDiscovery"
#"discovery/PassiveDiscovery1"
#"discovery/PassiveDiscovery2"
#"discovery/PeriodicHelloDiscovery"
#"discovery/HybridHelloDiscovery"
#"discovery/PeriodicJointDiscovery"
#"discovery/PeriodicDisjointDiscovery"
#"discovery/HybridDisjointDiscovery"
#"discovery/HybridHelloPeriodicHackDiscovery"
#"discovery/PeriodicHelloHybridHackDiscovery"
#"discovery/EchoDiscovery1"
#"discovery/EchoDiscovery2"
#"discovery/EchoDiscovery3"
#"discovery/OLSRDiscovery"
"discovery/LENWBDiscovery"
)

i=1
for t in ${TESTS[@]}; do
    #EXP="exp"$i
    EXP=$t
    i=$((i+1))
    EXE="../bin/discovery_test -d ../experiments/configs/$t.conf -a ../experiments/configs/discovery_app.conf -o ../topologies/exp1/"
    sudo ./test_algortihm_local.sh $PIS $DURATION $EXP "$EXE"
done
