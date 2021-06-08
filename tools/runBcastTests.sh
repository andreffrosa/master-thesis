#!/bin/bash

DISCOVERY_PERIOD="5" #s before running and start to broadcast
DURATION="600" #s duration of exp
COOL_PERIOD="60" #s time to wait before kill while not sending new messages
SLEEP="2000" #ms interval between broadcast
RUNS="1" # number of runs
WAIT_TIME="2" #s time between each command
RASPIS="24"

#use overlay topology
OVERLAY="-o"

EXP_TIME=$[$DISCOVERY_PERIOD+$DURATION+$COOL_PERIOD]

YGGHOME=/home/yggdrasil/
#DIR_PATH=/home/andreffrosa
#FRAMEWORK_EXE=/bin/bcast_test
DIR_PATH=/home/akos/nabas/
FRAMEWORK_EXE=bin/bcast_test

#0.5 probability of new broadcast period
ARGS="-i "$[$DISCOVERY_PERIOD*1000]" -s "$SLEEP" -p 0.5 -b -d "$DURATION" "$OVERLAY

FRAMEWORK=$DIR_PATH$FRAMEWORK_EXE' '$ARGS
RESULTS_DIR=$YGGHOME"bcast_results/"

#Modules

#RELATED WORK
FLOODING="-r -1"
GOSSIP="-r -2 -a 80" #-a probability of retransmit msg (paper says 80 is the best value)
COUNTING="-r -3 -a 2" #-a number of repetitions before cancel (paper says 2 is the best value)

#OUR WORK (the best work)
#NABA_0 : Counting with #Neighbours
NABA_0_0="-r 0 -v 0 -a 2" #counts the all msgs
NABA_0_1="-r 0 -v 1 -a 2" #counts msgs from diff parents (parent is the two-hop retrasmistion node)
NABA_0_2="-r 0 -v 2 -a 2" #still retransmits (with decressing prob) after limit (-a 2)

#NABA_1 : Checks neighbour relation
NABA_1="-r 1"

#NABA_2 : Checks neighbour relations + retransmits if no implicit ack (times phases)
NABA_2="-r 2"

#NABA_3 : Checks neighbour relations + cancel retrasmistion if all neighs should have msg
NABA_3="-r 3"

NULL_TIMER="-t 0" #does not wait
RANDOM_TIMER="-t 1" #waits for random time
NEIGH_TIMER="-t 2" #waits for time:neights
FIXED_TIMER="-t 3" #waits for a fixed time

#Experiments
EXP_COUNTER=0

#Flooding
EXPERIMENTS[$EXP_COUNTER]=$FRAMEWORK' '$FLOODING' '$NULL_TIMER
EXP_NAME[$EXP_COUNTER]="Flooding+Null_Timer"
((EXP_COUNTER++))


#Gossip
EXPERIMENTS[$EXP_COUNTER]=$FRAMEWORK' '$GOSSIP' '$NULL_TIMER
EXP_NAME[$EXP_COUNTER]="Gossip+Null_Timer"
((EXP_COUNTER++))


#Counting
EXPERIMENTS[$EXP_COUNTER]=$FRAMEWORK' '$COUNTING' '$NULL_TIMER
EXP_NAME[$EXP_COUNTER]="Counting+Null_Timer"
((EXP_COUNTER++))


#NABA-v0_0
EXPERIMENTS[$EXP_COUNTER]=$FRAMEWORK' '$NABA_0_0' '$NULL_TIMER
EXP_NAME[$EXP_COUNTER]="Naba-v0_0+Null_Timer"
((EXP_COUNTER++))


#NABA-v0_1
EXPERIMENTS[$EXP_COUNTER]=$FRAMEWORK' '$NABA_0_1' '$NULL_TIMER
EXP_NAME[$EXP_COUNTER]="Naba-v0_1+Null_Timer"
((EXP_COUNTER++))


#NABA-v0_2
EXPERIMENTS[$EXP_COUNTER]=$FRAMEWORK' '$NABA_0_2' '$NULL_TIMER
EXP_NAME[$EXP_COUNTER]="Naba-v0_2+Null_Timer"
((EXP_COUNTER++))

#NABA-v1
EXPERIMENTS[$EXP_COUNTER]=$FRAMEWORK' '$NABA_1' '$NULL_TIMER
EXP_NAME[$EXP_COUNTER]="Naba-v1+Null_Timer"
((EXP_COUNTER++))


#NABA-v2
PHASES=2
EXPERIMENTS[$EXP_COUNTER]=$FRAMEWORK' '$NABA_2" -a $PHASES"' '$NULL_TIMER
EXP_NAME[$EXP_COUNTER]="Naba-v2_$PHASES+Null_Timer"
((EXP_COUNTER++))


#NABA-v3
EXPERIMENTS[$EXP_COUNTER]=$FRAMEWORK' '$NABA_3" -a $PHASES"' '$NULL_TIMER
EXP_NAME[$EXP_COUNTER]="Naba-v3_$PHASES+Null_Timer"
((EXP_COUNTER++))

n_exp=${#EXPERIMENTS[@]}
#------------------------------------------------------------------

run_command() {
  now=$(date +"%T")
  echo "$now : Executing Command: "${YGGHOME}$1 "$2"
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
  local out=$(${YGGHOME}cmd/cmdbuildtree 127.0.0.1 5000)
}

check_tree() {
  local out=$(${YGGHOME}cmd/cmdchecktree 127.0.0.1 5000 | grep "raspi" | wc -l)
  return $out
}

disable_announces() {
  local out=$(${YGGHOME}cmd/cmddisablediscovery 127.0.0.1 5000 | grep "raspi" | wc -l)
  return $out
}

enable_announces() {
  local out=$(${YGGHOME}cmd/cmdenablediscovery 127.0.0.1 5000 | grep "raspi" | wc -l)
  return $out
}

start_experience() {
  run_command "cmd/cmdexecuteexperience 127.0.0.1 5000" "$1"
  echo "Start Experience: "$(check_command $?)
}

stop_experience() {
  run_command "cmd/cmdterminateexperience 127.0.0.1 5000" "$1"
  echo "Stop Experience: "$(check_command $?)
}

change_value() {
  run_command "cmd/cmdchangevalue 127.0.0.1 5000" "$1"
  echo "Change Value: "$(check_command $?)
}

change_link() {
  run_command "cmd/cmdchangelink 127.0.0.1 5000" "$1"
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

#

function timeToString() # $1 in seconds
{
    retval='ERROR'
    local arg1=$1

    if [ $1 -lt 60 ]
    then
        retval="Total execution time: $arg1 s"
    elif [ $arg1 -lt 3600 ]
    then
        retval=$(bc -l <<< "scale=2; ""$arg1/60")" min"
    elif [ $arg1 -lt 86400 ]
    then
        retval=$(bc -l <<< "scale=2; ""$arg1/3600")" h"
    else
        retval=$(bc -l <<< "scale=2; ""$arg1/86400")" days"
    fi

    #echo "$retval"
}

#begin nofaults test

echo "------------------------------------------------------"
echo "---------------------- BEGIN -------------------------"

echo "Discovery period: $DISCOVERY_PERIOD s"

timeToString $EXP_TIME
echo "Experiment time: $retval"

echo "Cool period: $COOL_PERIOD s"

echo "Number of runs: $RUNS"

TOTAL_TIME=$["(($RUNS*$n_exp*($EXP_TIME + 2*$WAIT_TIME)) + 3*$WAIT_TIME)"]
timeToString $TOTAL_TIME
echo "Total execution time: "$retval

echo setting up tree...
setup_tree

sleep $WAIT_TIME

check_tree
echo "Check Support tree: "$(check_command $?)

sleep $WAIT_TIME

echo "------------------------------------------------------"
echo "----------------- BEGIN NOFAULTS ---------------------"

for (( r=0; r < $RUNS; r++ )); do

echo ""
echo "------------------------------------------------------"
echo "-------------------------- run $r --------------------"
echo ""

for (( i=0; i < $n_exp; i++ )); do

  echo ""
  echo "----------------- ${EXP_NAME[$i]} run $r starting... ----------------"
  echo ""
  echo "setting up tree..."
  setup_tree

  sleep $WAIT_TIME

  check_tree
  echo "Check Support tree: "$(check_command $?)

  sleep $WAIT_TIME

  #run_command("sudo mkdir ${EXP_NAME[i]}")


  echo "start ${EXPERIMENTS[i]}"
  start_experience "${EXPERIMENTS[i]}"

  echo "sleep $EXP_TIME s"
  sleep $EXP_TIME

  stop_experience "$RESULTS_DIR${EXP_NAME[i]}_run$r.txt"
  echo "$RESULTS_DIR${EXP_NAME[i]}_run$r.txt"

  echo ""
  echo "-------- ${EXP_NAME[$i]} run $r ended waiting $WAIT_TIME seconds.. ---------"
  echo ""

done
done
echo "------------------------------------------------------"
echo "----------------- ENDED NOFAULTS ---------------------"
echo ""

sleep $WAIT_TIME
