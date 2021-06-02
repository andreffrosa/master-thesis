#!/bin/bash

YGGHOME="/home/andreffrosa/"
#DIR_PATH="/home/andreffrosa/"
RESULTS_DIR=$YGGHOME"discovery_results/"

RUNS=1
WAIT_TIME=5 #s
RASPIS=2 # TODO

#SLEEP_S=5 #s
DURATION=$["1*60"] #s
#EXP_TIME=$(expr $SLEEP_S + $DURATION)
EXP_TIME=$DURATION

#echo "EXP_TIME=$EXP_TIME"

#EXE="../bin/discovery_test -d ../experiments/configs/$t.conf -a ../experiments/configs/discovery_app.conf -o ../topologies/exp1/"

#CONFIGS="experiments/configs/"

get_exe() {
    A="../bin/discovery_test -d ../experiments/configs/discovery/$1.conf -a ../experiments/configs/discovery_app.conf"
    if [ "$2" = "" ]; then
        B=""
    else
        B="-o ../topologies/$2/"
    fi
    echo $A" "$B" -l $RESULTS_DIR$3"
}

TESTS=(
#"discovery/NoDiscovery"
#"discovery/PassiveDiscovery1"
#"discovery/PassiveDiscovery2"
#"discovery/PeriodicHelloDiscovery"
#"discovery/HybridHelloDiscovery"
"PeriodicJointDiscovery"
"PeriodicDisjointDiscovery"
#"discovery/HybridDisjointDiscovery"
#"discovery/HybridHelloPeriodicHackDiscovery"
#"discovery/PeriodicHelloHybridHackDiscovery"
"EchoDiscovery1"
"EchoDiscovery2"
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

N_EXP=${#TESTS[@]}

#------------------------------------------------------------------

run_command() {
  echo "Executing Command: "${YGGHOME}$1 "$2"
  local out=$(${YGGHOME}$1 "$2" | grep "raspi" | wc -l)
  return $out
}

check_command() {
  if [ $1 = $RASPIS ];
  then
    echo "OK"
  else
    echo ${1}": NOT OK"
  fi
}

setup_tree() {
  local out=$(${YGGHOME}bin/cmdbuildtree 127.0.0.1 5000)
}

check_tree() {
  local out=$(${YGGHOME}bin/cmdchecktree 127.0.0.1 5000 | grep "raspi" | wc -l)
  return $out
}

disable_announces() {
  local out=$(${YGGHOME}bin/cmddisablediscovery 127.0.0.1 5000 | grep "raspi" | wc -l)
  return $out
}

enable_announces() {
  local out=$(${YGGHOME}bin/cmdenablediscovery 127.0.0.1 5000 | grep "raspi" | wc -l)
  return $out
}

start_experience() {
  run_command "bin/cmdexecuteexperience 127.0.0.1 5000" "$1"
  echo "Start Experience: "$(check_command $?)
}

stop_experience() {
  run_command "bin/cmdterminateexperience 127.0.0.1 5000" "$1"
  echo "Stop Experience: "$(check_command $?)
}

change_value() {
  run_command "bin/cmdchangevalue 127.0.0.1 5000" "$1"
  echo "Change Value: "$(check_command $?)
}

change_link() {
  run_command "bin/cmdchangelink 127.0.0.1 5000" "$1"
  echo "Change Link: "$(check_command $?)
}

getroot() {
    local rand=$((1 + RANDOM % 24))
    if (($rand < 10)); then
        echo "raspi-0$rand"
    else
        echo "raspi-$rand"
    fi
}

getrandompi() {
    local rand=$((1 + RANDOM % 24))
    echo $rand
}

getrandomnumber() {
    local rand=$((1 + RANDOM % 200))
    echo $rand
}

function show_time() {
    num=$1
    min=0
    hour=0
    day=0
    if((num>59));then
        ((sec=num%60))
        ((num=num/60))
        if((num>59));then
            ((min=num%60))
            ((num=num/60))
            if((num>23));then
                ((hour=num%24))
                ((day=num/24))
            else
                ((hour=num))
            fi
        else
            ((min=num))
        fi
    else
        ((sec=num))
    fi
    echo "$day"d "$hour"h "$min"m "$sec"s
}

function timeToString() # $1 in seconds
{
    #retval='ERROR'
    #local arg1=$1

    #if [ "$arg1" -lt 60 ]
    #then
    #    retval="Total execution time: $arg1 s"
    #elif [ "$arg1" -lt 3600 ]
    #then
    #    retval=$(bc -l <<< "scale=2; ""$arg1/60")" min"
    #elif [ "$arg1" -lt 86400 ]
    #then
    #    retval=$(bc -l <<< "scale=2; ""$arg1/3600")" h"
    #else
    #    retval=$(bc -l <<< "scale=2; ""$arg1/86400")" days"
    #fi

    #retval="$1 s"
    #echo "$retval"
    show_time $1
}

##################################################################


echo "------------------------------------------------------"
echo "---------------------- BEGIN -------------------------"

echo ""

echo "Duration per test: $(timeToString $EXP_TIME)"

echo "Number of runs: $RUNS"

TOTAL_TIME=$["(($RUNS*$N_EXP*($EXP_TIME + 3*$WAIT_TIME)) + 3*$WAIT_TIME)"]
echo "Total execution time: "$(timeToString $TOTAL_TIME)

echo ""

############################################################

echo "Setting up tree..."
setup_tree

sleep $WAIT_TIME

check_tree
echo "Check Support tree: "$(check_command $?)

sleep $WAIT_TIME

exit

#echo "Creating results dir $RESULTS_DIR ..."
#run_command "sudo mkdir $RESULTS_DIR"

echo ""

############################################################

echo "------------------------------------------------------"
echo "---------------------- START -------------------------"

echo ""

for (( r=1; r < $RUNS+1; r++ )); do

    echo "------------------------------------------------------"
    echo "-------------------- RUN $r/$RUNS ------------------------"

    echo ""

    S_TESTS=( $(shuf -e "${TESTS[@]}") )

    #echo $S_TESTS

    for (( i=0; i < $N_EXP; i++ )); do

        echo ""
        echo "-------------- ${S_TESTS[$i]} RUN $r/$RUNS -----------"
        echo ""

        echo "Setting up tree..."
        setup_tree

        sleep $WAIT_TIME

        check_tree
        echo "Check Support tree: "$(check_command $?)

        sleep $WAIT_TIME

        echo ""

        LOG="${S_TESTS[$i]}-r$r"
        CMD=$(get_exe ${S_TESTS[$i]} "" $LOG"/")

        echo "start> $CMD"
        start_experience "$CMD"

        echo ""

        echo "sleep $EXP_TIME s"
        sleep $EXP_TIME

        stop_experience "$RESULTS_DIR$LOG.log"
        echo "$RESULTS_DIR$LOG.log"

        echo ""

        echo ""
        echo "-------- ${S_TESTS[$i]} RUN $r/$RUNS ended ---------"
        echo ""

        echo "waiting $WAIT_TIME s ..."
        sleep $WAIT_TIME
    done

    echo ""
done

echo ""

echo "------------------------------------------------------"
echo "---------------------- ENDED -------------------------"
echo "------------------------------------------------------"

sleep $WAIT_TIME
