#!/bin/bash

YGGHOME=/home/yggdrasil/

DIR_PATH=/home/andreffrosa

FRAMEWORK=/bin/bcast_test

ARGS="-i 1000 -s 1000 -p 0.5 -b"

FLOODING="-r -1"
NABA_0_0="-r 0 -v 0"
NABA_0_1="-r 0 -v 1"
NABA_0_2="-r 0 -v 2"
NABA_1="-r 1"
NABA_2="-r 2"
NABA_3="-r 3"

NULL_TIMER="-t 0"
RANDOM_TIMER="-t 1"
NEIGH_TIMER="-t 2"

EXPERIMENTS[6]=$DIR_PATH$FRAMEWORK' '$ARGS' '$NABA_1' '$NULL_TIMER
RESULTS[6]=$YGGHOME"bcast_results/""NABA-v1+NULL_TIMER_1"

EXPERIMENTS[7]=$DIR_PATH$FRAMEWORK' '$ARGS' '$NABA_1' '$NULL_TIMER
RESULTS[7]=$YGGHOME"bcast_results/""NABA-v1+NULL_TIMER_2"

EXPERIMENTS[8]=$DIR_PATH$FRAMEWORK' '$ARGS' '$NABA_1' '$NULL_TIMER
RESULTS[8]=$YGGHOME"bcast_results/""NABA-v1+NULL_TIMER_3"

EXPERIMENTS[9]=$DIR_PATH$FRAMEWORK' '$ARGS' '$NABA_1' '$RANDOM_TIMER
RESULTS[9]=$YGGHOME"bcast_results/""NABA-v1+RANDOM_TIMER_1"

EXPERIMENTS[10]=$DIR_PATH$FRAMEWORK' '$ARGS' '$NABA_1' '$RANDOM_TIMER
RESULTS[10]=$YGGHOME"bcast_results/""NABA-v1+RANDOM_TIMER_2"

EXPERIMENTS[11]=$DIR_PATH$FRAMEWORK' '$ARGS' '$NABA_1' '$RANDOM_TIMER
RESULTS[11]=$YGGHOME"bcast_results/""NABA-v1+RANDOM_TIMER_3"

EXPERIMENTS[12]=$DIR_PATH$FRAMEWORK' '$ARGS' '$NABA_1' '$NEIGH_TIMER
RESULTS[12]=$YGGHOME"bcast_results/""NABA-v1+NEIGH_TIMER_1"

EXPERIMENTS[13]=$DIR_PATH$FRAMEWORK' '$ARGS' '$NABA_1' '$NEIGH_TIMER
RESULTS[13]=$YGGHOME"bcast_results/""NABA-v1+NEIGH_TIMER_2"

EXPERIMENTS[14]=$DIR_PATH$FRAMEWORK' '$ARGS' '$NABA_1' '$NEIGH_TIMER
RESULTS[14]=$YGGHOME"bcast_results/""NABA-v1+NEIGH_TIMER_3"

EXP_TIME=600
WAIT_TIME=2

RASPIS="24"

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

#begin nofaults test

echo "------------------------------------------------------"
echo "---------------------- BEGIN -------------------------"

discov_period=1
#msg_per_fault=10

n_runs=3

echo "Discovery period: $discov_period"
echo "Number of runs: $n_runs"

echo setting up tree...
setup_tree

sleep $WAIT_TIME

check_tree
echo "Check Support tree: "$(check_command $?)

sleep $WAIT_TIME

echo "------------------------------------------------------"
echo "----------------- BEGIN NOFAULTS ---------------------"

#for k in 1 2 3; do

echo ""
echo "------------------------------------------------------"
echo "run $k"
echo ""

for i in {6..14}
do

  echo ""
  echo "----------------- EXP $i starting.. ----------------"
  echo ""
  echo "setting up tree..."
  setup_tree

  sleep $WAIT_TIME

  check_tree
  echo "Check Support tree: "$(check_command $?)

  sleep $WAIT_TIME

  #run_command("sudo mkdir ${RESULTS[i]}")  

  start_experience "${EXPERIMENTS[i]}"

  sleep $EXP_TIME

  stop_experience "${RESULTS[i]}.txt"

  echo ""
  echo "---- EXP $i ended waiting $WAIT_TIME seconds.. -----"
  echo ""

done
#done
echo "------------------------------------------------------"
echo "----------------- ENDED NOFAULTS ---------------------"

sleep $WAIT_TIME
