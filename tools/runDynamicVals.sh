#!/bin/bash

YGGHOME=/home/yggdrasil/

MULTITEST=/home/yggdrasil/bin/aggMultiTest
FLOWTEST=/home/yggdrasil/bin/aggFlowTest
GAPTEST=/home/yggdrasil/bin/aggGapTest
GAPBCASTTEST=/home/yggdrasil/bin/aggGapBcastTest
LIMOTEST=/home/yggdrasil/bin/aggLimoTest

SRDS18=/home/yggdrasil/srds18/

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

discov_period=$1
msg_per_fault=$2

n_runs=$3
start_on=$4
end_on=$5

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


echo ""

echo "------------------------------------------------------"
echo "----------- START NOFAULTS DYNAMIC VALUES ------------"
echo ""

EXP_TIME=$(($EXP_TIME/2))
#begin nofaults with dynamic values
#1 node change values
#3 node change values
#6 node change values
#12 nodes change values
#24 nodes change values
#already done: 1 3 6 12 24
for changes in 1 12 24; do


  echo "------------------------------------------------------"
  echo "-------------- CHANGE $changes VALUES ----------------"

  for i in $(seq 1 $n_runs); do
    pisnewval=""
    for pi in $(shuf -i 1-24 -n $changes); do
      newval=$(getrandomnumber)

      echo "To change value of Pi: $pi to: $newval"
      pisnewval+="$pi $newval "
    done
    arg=${pisnewval:0:$((${#pisnewval}-1))}

    echo ""
    echo "----------------- MULTI $i starting.. ----------------"
    echo ""

    echo "setting up tree..."
    setup_tree

    sleep $WAIT_TIME

    check_tree
    echo "Check Support tree: "$(check_command $?)

    sleep $WAIT_TIME

    start_experience "$MULTITEST $discov_period $msg_per_fault"

    sleep $EXP_TIME

    STARTTIME=$(date +%s)

    change_value "$arg"

    ENDTIME=$(date +%s)
    elapsed=$(($ENDTIME - $STARTTIME))

    sleep $((($EXP_TIME) - $elapsed))

    stop_experience "${SRDS18}nofaults_dynamic_${changes}_aggMultiTest_${discov_period}_${msg_per_fault}_$i"

    echo ""
    echo "---- MULTI $i ended waiting $WAIT_TIME seconds.. -----"
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

    start_experience "$FLOWTEST $discov_period $msg_per_fault"

    sleep $EXP_TIME

    STARTTIME=$(date +%s)

    change_value "$arg"

    ENDTIME=$(date +%s)
    elapsed=$(($ENDTIME - $STARTTIME))

    sleep $((($EXP_TIME) - $elapsed))

    stop_experience "${SRDS18}nofaults_dynamic_${changes}_aggFlowTest_${discov_period}_${msg_per_fault}_$i"

    echo ""
    echo "----- FLOW $i ended waiting $WAIT_TIME seconds.. -----"
    echo ""

    sleep $WAIT_TIME

    echo ""
    echo "----------------- LIMO $i starting.. -----------------"
    echo ""

    setup_tree

    sleep $WAIT_TIME

    check_tree
    echo "Check Support tree: "$(check_command $?)

    sleep $WAIT_TIME

    start_experience "$LIMOTEST $discov_period $msg_per_fault"

    sleep $EXP_TIME

    STARTTIME=$(date +%s)

    change_value "$arg"

    ENDTIME=$(date +%s)
    elapsed=$(($ENDTIME - $STARTTIME))

    sleep $((($EXP_TIME) - $elapsed))

    stop_experience "${SRDS18}nofaults_dynamic_${changes}_aggLimoTest_${discov_period}_${msg_per_fault}_$i"

    echo ""
    echo "----- LIMO $i ended waiting $WAIT_TIME seconds.. -----"
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
    gap_root="raspi-01"

    start_experience "$GAPTEST $gap_root $discov_period $msg_per_fault"
    echo "root is $gap_root"

    sleep $EXP_TIME

    STARTTIME=$(date +%s)

    change_value "$arg"

    ENDTIME=$(date +%s)
    elapsed=$(($ENDTIME - $STARTTIME))

    sleep $((($EXP_TIME) - $elapsed))

    stop_experience "${SRDS18}nofaults_dynamic_${changes}_aggGapTest_root_${gap_root}_${discov_period}_${msg_per_fault}_$i"

    echo ""
    echo "----- GAP $i ended waiting $WAIT_TIME seconds.. ------"
    echo ""

    sleep $WAIT_TIME

    echo ""
    echo "-------------- GAPBCAST $i starting.. ----------------"
    echo ""

    setup_tree

    sleep $WAIT_TIME

    check_tree
    echo "Check Support tree: "$(check_command $?)

    sleep $WAIT_TIME

    start_experience "$GAPBCASTTEST $gap_root $discov_period $msg_per_fault"
    echo "root is $gap_root"

    sleep $EXP_TIME

    STARTTIME=$(date +%s)

    change_value "$arg"

    ENDTIME=$(date +%s)
    elapsed=$(($ENDTIME - $STARTTIME))

    sleep $((($EXP_TIME) - $elapsed))

    stop_experience "${SRDS18}nofaults_dynamic_${changes}_aggGapBcastTest_root_${gap_root}_${discov_period}_${msg_per_fault}_$i"

    echo ""
    echo "--- GAPBCAST $i ended waiting $WAIT_TIME seconds.. ---"
    echo ""

    sleep $WAIT_TIME
  done
  echo "---------------------- ENDED -------------------------"
  echo "-------------- CHANGE $changes VALUES ----------------"
  echo ""
done

echo "------------------------------------------------------"
echo "----------- ENDED NOFAULTS DYNAMIC VALUES ------------"
echo ""
