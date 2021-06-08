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
echo "----------- START NOFAULTS DYNAMIC VALUES ------------"
echo ""


changes=1
  echo "------------------------------------------------------"
  echo "-------------- CHANGE $changes VALUES ----------------"

  i=1
  arg="12 20"

  lowest_tree=0
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

  change_value "$arg"

  ENDTIME=$(date +%s)
  elapsed=$(($ENDTIME - $STARTTIME))

  sleep $((($EXP_TIME) - $elapsed))

  stop_experience "${SRDS18}nofaults_dynamic_${changes}_aggMultiTest_${lowest_tree}_${active_neigh}_$i"

  echo ""
  echo "---- MULTI $i ended waiting $WAIT_TIME seconds.. -----"
  echo "---------lowest tree = $lowest_tree --- active neigh = $active_neigh -------"
  echo ""

  sleep $WAIT_TIME

  i=2
  arg="10 138"

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

  change_value "$arg"

  ENDTIME=$(date +%s)
  elapsed=$(($ENDTIME - $STARTTIME))

  sleep $((($EXP_TIME) - $elapsed))

  stop_experience "${SRDS18}nofaults_dynamic_${changes}_aggMultiTest_${lowest_tree}_${active_neigh}_$i"

  echo ""
  echo "---- MULTI $i ended waiting $WAIT_TIME seconds.. -----"
  echo "---------lowest tree = $lowest_tree --- active neigh = $active_neigh -------"
  echo ""

  sleep $WAIT_TIME

  i=3
  arg="5 100"

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

  change_value "$arg"

  ENDTIME=$(date +%s)
  elapsed=$(($ENDTIME - $STARTTIME))

  sleep $((($EXP_TIME) - $elapsed))

  stop_experience "${SRDS18}nofaults_dynamic_${changes}_aggMultiTest_${lowest_tree}_${active_neigh}_$i"

  echo ""
  echo "---- MULTI $i ended waiting $WAIT_TIME seconds.. -----"
  echo "---------lowest tree = $lowest_tree --- active neigh = $active_neigh -------"
  echo ""

  sleep $WAIT_TIME


  changes=12
    echo "------------------------------------------------------"
    echo "-------------- CHANGE $changes VALUES ----------------"

    i=1
    arg="1 54 20 147 9 138 16 2 22 185 10 76 13 11 7 24 19 91 23 40 8 104 11 44"

    lowest_tree=0
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

    change_value "$arg"

    ENDTIME=$(date +%s)
    elapsed=$(($ENDTIME - $STARTTIME))

    sleep $((($EXP_TIME) - $elapsed))

    stop_experience "${SRDS18}nofaults_dynamic_${changes}_aggMultiTest_${lowest_tree}_${active_neigh}_$i"

    echo ""
    echo "---- MULTI $i ended waiting $WAIT_TIME seconds.. -----"
    echo "---------lowest tree = $lowest_tree --- active neigh = $active_neigh -------"
    echo ""

    sleep $WAIT_TIME

    i=2
    arg="11 69 13 176 23 88 21 130 6 59 17 193 16 118 8 111 19 4 1 152 12 177 2 92"

    lowest_tree=0
    active_neigh=1
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

    change_value "$arg"

    ENDTIME=$(date +%s)
    elapsed=$(($ENDTIME - $STARTTIME))

    sleep $((($EXP_TIME) - $elapsed))

    stop_experience "${SRDS18}nofaults_dynamic_${changes}_aggMultiTest_${lowest_tree}_${active_neigh}_$i"

    echo ""
    echo "---- MULTI $i ended waiting $WAIT_TIME seconds.. -----"
    echo "---------lowest tree = $lowest_tree --- active neigh = $active_neigh -------"
    echo ""

    sleep $WAIT_TIME

    changes=24
      echo "------------------------------------------------------"
      echo "-------------- CHANGE $changes VALUES ----------------"

      i=1
      arg="6 28 18 45 8 160 3 74 12 76 10 152 19 9 7 125 9 115 21 55 22 61 4 132 14 168 15 69 16 59 5 196 1 106 11 135 23 11 17 113 13 188 2 172 24 35 20 173"

      lowest_tree=0
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

      change_value "$arg"

      ENDTIME=$(date +%s)
      elapsed=$(($ENDTIME - $STARTTIME))

      sleep $((($EXP_TIME) - $elapsed))

      stop_experience "${SRDS18}nofaults_dynamic_${changes}_aggMultiTest_${lowest_tree}_${active_neigh}_$i"

      echo ""
      echo "---- MULTI $i ended waiting $WAIT_TIME seconds.. -----"
      echo "---------lowest tree = $lowest_tree --- active neigh = $active_neigh -------"
      echo ""

      sleep $WAIT_TIME

      i=3
      arg="5 48 18 174 21 130 4 25 23 8 1 161 14 63 17 86 8 67 3 139 16 118 6 153 7 140 19 151 13 13 24 71 20 129 22 186 15 46 10 191 9 106 11 43 12 2 2 122"

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

      change_value "$arg"

      ENDTIME=$(date +%s)
      elapsed=$(($ENDTIME - $STARTTIME))

      sleep $((($EXP_TIME) - $elapsed))

      stop_experience "${SRDS18}nofaults_dynamic_${changes}_aggMultiTest_${lowest_tree}_${active_neigh}_$i"

      echo ""
      echo "---- MULTI $i ended waiting $WAIT_TIME seconds.. -----"
      echo "---------lowest tree = $lowest_tree --- active neigh = $active_neigh -------"
      echo ""

      sleep $WAIT_TIME

      lowest_tree=0
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

      change_value "$arg"

      ENDTIME=$(date +%s)
      elapsed=$(($ENDTIME - $STARTTIME))

      sleep $((($EXP_TIME) - $elapsed))

      stop_experience "${SRDS18}nofaults_dynamic_${changes}_aggMultiTest_${lowest_tree}_${active_neigh}_$i"

      echo ""
      echo "---- MULTI $i ended waiting $WAIT_TIME seconds.. -----"
      echo "---------lowest tree = $lowest_tree --- active neigh = $active_neigh -------"
      echo ""

      sleep $WAIT_TIME


echo "------------------------------------------------------"
echo "----------------- ENDED NOFAULTS ---------------------"

sleep $WAIT_TIME
