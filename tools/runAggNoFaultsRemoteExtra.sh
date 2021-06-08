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

#EXP_TIME=$(($EXP_TIME/2))

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

gap_root="raspi-01"

echo "------------------------------------------------------"
echo "----------------- BEGIN NOFAULTS ---------------------"

i=4

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

stop_experience "${SRDS18}nofaults_remote_aggMultiTest_${lowest_tree}_$i"

echo ""
echo "---- MULTI $i ended waiting $WAIT_TIME seconds.. -----"
echo "---------lowest tree = $lowest_tree ----------"
echo ""

sleep $WAIT_TIME


i=5

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

stop_experience "${SRDS18}nofaults_remote_aggMultiTest_${lowest_tree}_$i"

echo ""
echo "---- MULTI $i ended waiting $WAIT_TIME seconds.. -----"
echo "---------lowest tree = $lowest_tree ----------"
echo ""

sleep $WAIT_TIME

echo ""
echo "---------------- FLOW $i starting.. ------------------"
echo ""
setup_tree

sleep $WAIT_TIME

check_tree
echo "Check Support tree: "$(check_command $?)

sleep $WAIT_TIME

start_experience "$FLOWTEST"

sleep $EXP_TIME

stop_experience "${SRDS18}nofaults_remote_aggFlowTest_$i"

echo ""
echo "----- FLOW $i ended waiting $WAIT_TIME seconds.. -----"
echo ""

sleep $WAIT_TIME

echo ""
echo "----------------- GAP $i starting.. ------------------"
echo ""

setup_tree

sleep $WAIT_TIME

check_tree
echo "Check Support tree: "$(check_command $?)

sleep $WAIT_TIME
#gap_root=$(getroot)
gap_root="raspi-01"

start_experience "$GAPTEST $gap_root"
echo "root is $gap_root"
sleep $EXP_TIME

stop_experience "${SRDS18}nofaults_remote_aggGapTest_root_${gap_root}_$i"

echo ""
echo "----- GAP $i ended waiting $WAIT_TIME seconds.. ------"
echo ""

sleep $WAIT_TIME

i=6

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

stop_experience "${SRDS18}nofaults_remote_aggMultiTest_${lowest_tree}_$i"

echo ""
echo "---- MULTI $i ended waiting $WAIT_TIME seconds.. -----"
echo "---------lowest tree = $lowest_tree ----------"
echo ""

sleep $WAIT_TIME

echo "------------------------------------------------------"
echo "----------------- ENDED NOFAULTS ---------------------"

sleep $WAIT_TIME
