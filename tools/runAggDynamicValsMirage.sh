#!/bin/bash

YGGHOME=/home/yggdrasil/

MULTITEST=/home/yggdrasil/bin/aggMultiTest
MULTITESTV2=/home/yggdrasil/bin/aggMultiTestV2
FLOWTEST=/home/yggdrasil/bin/aggFlowTest
GAPTEST=/home/yggdrasil/bin/aggGapTest
GAPBCASTTEST=/home/yggdrasil/bin/aggGapBcastTest
LIMOTEST=/home/yggdrasil/bin/aggLimoTest

SRDS18=/home/yggdrasil/aggregation/

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
msg_per_fault=10

n_runs=3

EXP_TIME=$(($EXP_TIME/2))

echo "Discovery period: $discov_period"
echo "Messages Per Fault: $msg_per_fault"
echo "Number of runs: $n_runs"

echo setting up tree...
setup_tree

sleep $WAIT_TIME

check_tree
echo "Check Support tree: "$(check_command $?)

sleep $WAIT_TIME

#echo "Disabling annouces.."
#disable_announces
#echo "Disable Announces: "$(check_command $?)


echo "------------------------------------------------------"
echo "----------------- START CHANGE VALS ------------------"
echo ""

vals[1]="7 97"
vals[12]="3 79 18 87 13 46 16 125 22 16 12 103 2 52 15 134 10 5 20 137 5 111 1 186"
vals[24]="23 161 7 88 13 179 4 84 10 161 17 37 20 103 16 176 14 101 12 53 19 57 15 186 5 87 18 88 21 152 2 69 8 136 9 112 22 154 11 119 24 82 6 135 3 132 1 39"

for changes in 1 12 24; do

  echo "------------------------------------------------------"
  echo "--------------- CHANGE $changes VALUES -----------------"

arg=${vals[$changes]}
echo ""
echo "to change values of: $arg"

for i in $(seq 1 $n_runs); do

  lowest_tree=0
  active_neigh=1
  echo ""
  echo "----------------- MULTI $i starting.. ----------------"
  echo "---------lowest tree = $lowest_tree ----------"
  echo ""
  echo "setting up tree..."
  setup_tree

  sleep $WAIT_TIME

  check_tree
  echo "Check Support tree: "$(check_command $?)

  sleep $WAIT_TIME

  start_experience "$MULTITEST $lowest_tree $active_neigh"

  sleep $EXP_TIME

  STARTTIME=$(date +%s)

  change_value "$arg"


  ENDTIME=$(date +%s)
  elapsed=$(($ENDTIME - $STARTTIME))

  sleep $((($EXP_TIME) - $elapsed))


  stop_experience "${SRDS18}nofaults_dynamic_${changes}_aggMultiTest_${lowest_tree}_$i"

  echo ""
  echo "---- MULTI $i ended waiting $WAIT_TIME seconds.. -----"
  echo "---------lowest tree = $lowest_tree ----------"
  echo ""

  sleep $WAIT_TIME

  lowest_tree=1
  active_neigh=1
  echo ""
  echo "----------------- MULTI $i starting.. ----------------"
  echo "---------lowest tree = $lowest_tree ----------"
  echo ""
  echo "setting up tree..."
  setup_tree

  sleep $WAIT_TIME

  check_tree
  echo "Check Support tree: "$(check_command $?)

  sleep $WAIT_TIME

  start_experience "$MULTITEST $lowest_tree $active_neigh"

  sleep $EXP_TIME

  STARTTIME=$(date +%s)

  change_value "$arg"


  ENDTIME=$(date +%s)
  elapsed=$(($ENDTIME - $STARTTIME))

  sleep $((($EXP_TIME) - $elapsed))

  stop_experience "${SRDS18}nofaults_dynamic_${changes}_aggMultiTest_${lowest_tree}_$i"

  echo ""
  echo "---- MULTI $i ended waiting $WAIT_TIME seconds.. -----"
  echo "---------lowest tree = $lowest_tree ----------"
  echo ""

  sleep $WAIT_TIME


done
done

echo "------------------------------------------------------"
echo "----------------- ENDED CHANGE VALS ---------------------"

sleep $WAIT_TIME
