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
"discovery/PeriodicJointDiscovery"
"discovery/PeriodicJointDiscovery2"
"discovery/PeriodicDisjointDiscovery"
"discovery/PeriodicDisjointDiscovery2"
#"discovery/HybridDisjointDiscovery"
#"discovery/HybridHelloPeriodicHackDiscovery"
#"discovery/PeriodicHelloHybridHackDiscovery"
"discovery/EchoDiscovery1"
"discovery/EchoDiscovery2"
#"discovery/EchoDiscovery3"
#"discovery/OLSRDiscovery"
#"discovery/LENWBDiscovery"
#"discovery/PeriodicJointDiscovery+Age"
#"discovery/PeriodicJointDiscovery+Hysteresis"
#"discovery/HybridDisjointDiscovery+Age"
#"discovery/HybridDisjointDiscovery+Hysteresis"
#"discovery/PassiveDiscovery1+Age"
#"discovery/PassiveDiscovery1+Hysteresis"
#"discovery/EchoDiscovery1+Age"
#"discovery/EchoDiscovery1+Hysteresis"
#"discovery/OLSRDiscovery+Age"
#"discovery/OLSRDiscovery+Hysteresis"
)

#ARR=( $(echo $EXP | sed 's/\// /g') )
#d1=${ARR[0]}
#d2=${ARR[1]}
#d0
L_DIR="../experiments/output/discovery/discovery_tests_"$(date +"%Y.%m.%d-%H.%M.%S")

rm -r "$L_DIR/" > /dev/null 2>&1
mkdir "$L_DIR/" > /dev/null 2>&1

i=1
for t in ${TESTS[@]}; do
    #EXP="exp"$i
    EXP=$t
    i=$((i+1))
    EXE="../bin/discovery_test -d ../experiments/configs/$t.conf -a ../experiments/configs/discovery_app.conf -o ../topologies/exp1/ -s 10000"
    sudo ./test_algortihm_local.sh $PIS $DURATION $EXP "$EXE" $L_DIR
done
