#!/bin/bash

YGGHOME=/home/yggdrasil/

MULTITEST=/home/yggdrasil/bin/aggMultiTest
FLOWTEST=/home/yggdrasil/bin/aggFlowTest
GAPTEST=/home/yggdrasil/bin/aggGapTest
GAPBCASTTEST=/home/yggdrasil/bin/aggGapBcastTest
LIMOTEST=/home/yggdrasil/bin/aggLimoTest

SRDS18=/home/yggdrasil/mirage/

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


EXP_TIME=$(($EXP_TIME/2))
echo "------------------------------------------------------"
echo "----------------- START LINK FAULTS ------------------"
echo ""

tokill=(2 7 23 22 18 17 4 1 7 9 19 16 21 20 9 14 24 15 2 6 4 6 3 4)

changes=1
  echo "------------------------------------------------------"
  echo "--------------- FAULT $changes LINKS -----------------"


  pis1=""
  for pi in ${tokill[@]:0:$(($changes*2))}; do
    pis1+="$pi "
  done

  pis=${pis1:0:$((${#pis1}-1))}
  echo "Pis to kill in this run: $pis"


  i=2
  lowest_tree=1
  active_neigh=0
  echo ""
  echo "----------------- MULTI $i starting.. ----------------"
  echo "---------lowest tree = $lowest_tree --- active neigh = $active_neigh -------"
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


  change_link "$pis"
  echo "Killed Pis: $pis"


  ENDTIME=$(date +%s)
  elapsed=$(($ENDTIME - $STARTTIME))

  sleep $((($EXP_TIME) - $elapsed))

  stop_experience "${SRDS18}link_faults_${changes}_aggMultiTest_${lowest_tree}_${active_neigh}_$i"

  echo ""
  echo "---- MULTI $i ended waiting $WAIT_TIME seconds.. -----"
  echo "---------lowest tree = $lowest_tree --- active neigh = $active_neigh -------"
  echo ""

  i=3
  lowest_tree=1
  active_neigh=0
  echo ""
  echo "----------------- MULTI $i starting.. ----------------"
  echo "---------lowest tree = $lowest_tree --- active neigh = $active_neigh -------"
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


  change_link "$pis"
  echo "Killed Pis: $pis"


  ENDTIME=$(date +%s)
  elapsed=$(($ENDTIME - $STARTTIME))

  sleep $((($EXP_TIME) - $elapsed))

  stop_experience "${SRDS18}link_faults_${changes}_aggMultiTest_${lowest_tree}_${active_neigh}_$i"

  echo ""
  echo "---- MULTI $i ended waiting $WAIT_TIME seconds.. -----"
  echo "---------lowest tree = $lowest_tree --- active neigh = $active_neigh -------"
  echo ""


echo "------------------------------------------------------"
echo "----------------- ENDED LINKFAULTS -------------------"
