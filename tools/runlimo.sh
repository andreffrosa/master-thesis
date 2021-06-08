#!/bin/bash

YGGHOME=/home/yggdrasil/

MULTITEST=/home/yggdrasil/bin/aggMultiTest
FLOWTEST=/home/yggdrasil/bin/aggFlowTest
GAPTEST=/home/yggdrasil/bin/aggGapTest
GAPBCASTTEST=/home/yggdrasil/bin/aggGapBcastTest
LIMOTEST=/home/yggdrasil/bin/aggLimoTest

SRDS18=/home/yggdrasil/srds18/

EXP_TIME=300
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
start_on=1
end_on=1

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

tokill=(7 23 17 4 11 18 21 22 20 19 2 14)

changes=1

echo "------------------------------------------------------"
echo "--------------- FAULT $changes NODES -----------------"

pis1=""
for pi in ${tokill[@]:0:$changes}; do
  pis1+="$pi "
done

pis=${pis1:0:$((${#pis1}-1))}
echo "Pis to kill in this run: $pis"
echo ""
echo "----------------- MULTI 1 starting.. ----------------"
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

stop_experience "${SRDS18}faults_${changes}_aggMultiTest_${discov_period}_${msg_per_fault}_1 $pis"
echo "Killed Pis: $pis"
ENDTIME=$(date +%s)
elapsed=$(($ENDTIME - $STARTTIME))

sleep $((($EXP_TIME) - $elapsed))

stop_experience "${SRDS18}faults_${changes}_aggMultiTest_${discov_period}_${msg_per_fault}_1"

echo ""
echo "---- MULTI 1 ended waiting $WAIT_TIME seconds.. -----"
echo ""

sleep $WAIT_TIME


echo ""
echo "----------------- LIMO 2 starting.. -----------------"
echo ""

setup_tree

sleep $WAIT_TIME

check_tree
echo "Check Support tree: "$(check_command $?)

sleep $WAIT_TIME

start_experience "$LIMOTEST $discov_period $msg_per_fault"

sleep $EXP_TIME

STARTTIME=$(date +%s)

stop_experience "${SRDS18}faults_${changes}_aggLimoTest_${discov_period}_${msg_per_fault}_2 $pis"
echo "Killed Pis: $pis"
ENDTIME=$(date +%s)
elapsed=$(($ENDTIME - $STARTTIME))

sleep $((($EXP_TIME) - $elapsed))

stop_experience "${SRDS18}faults_${changes}_aggLimoTest_${discov_period}_${msg_per_fault}_2"

echo ""
echo "----- LIMO 2 ended waiting $WAIT_TIME seconds.. -----"
echo ""

sleep $WAIT_TIME

echo ""
echo "----------------- LIMO 3 starting.. -----------------"
echo ""

setup_tree

sleep $WAIT_TIME

check_tree
echo "Check Support tree: "$(check_command $?)

sleep $WAIT_TIME

start_experience "$LIMOTEST $discov_period $msg_per_fault"

sleep $EXP_TIME

STARTTIME=$(date +%s)

stop_experience "${SRDS18}faults_${changes}_aggLimoTest_${discov_period}_${msg_per_fault}_3 $pis"
echo "Killed Pis: $pis"
ENDTIME=$(date +%s)
elapsed=$(($ENDTIME - $STARTTIME))

sleep $((($EXP_TIME) - $elapsed))

stop_experience "${SRDS18}faults_${changes}_aggLimoTest_${discov_period}_${msg_per_fault}_3"

echo ""
echo "----- LIMO 3 ended waiting $WAIT_TIME seconds.. -----"
echo ""

sleep $WAIT_TIME


echo "---------------------- ENDED -------------------------"
echo "--------------- FAULT $changes NODES -----------------"

echo ""
changes=6
echo ""

echo "------------------------------------------------------"
echo "--------------- FAULT $changes NODES -----------------"

pis1=""
for pi in ${tokill[@]:0:$changes}; do
  pis1+="$pi "
done

pis=${pis1:0:$((${#pis1}-1))}
echo "Pis to kill in this run: $pis"



echo "---------------------- ENDED -------------------------"
echo "--------------- FAULT $changes NODES -----------------"

echo ""
changes=12
echo ""

echo "------------------------------------------------------"
echo "--------------- FAULT $changes NODES -----------------"

pis1=""
for pi in ${tokill[@]:0:$changes}; do
  pis1+="$pi "
done

pis=${pis1:0:$((${#pis1}-1))}
echo "Pis to kill in this run: $pis"

echo ""
echo "----------------- LIMO 2 starting.. -----------------"
echo ""

setup_tree

sleep $WAIT_TIME

check_tree
echo "Check Support tree: "$(check_command $?)

sleep $WAIT_TIME

start_experience "$LIMOTEST $discov_period $msg_per_fault"

sleep $EXP_TIME

STARTTIME=$(date +%s)

stop_experience "${SRDS18}faults_${changes}_aggLimoTest_${discov_period}_${msg_per_fault}_2 $pis"
echo "Killed Pis: $pis"
ENDTIME=$(date +%s)
elapsed=$(($ENDTIME - $STARTTIME))

sleep $((($EXP_TIME) - $elapsed))

stop_experience "${SRDS18}faults_${changes}_aggLimoTest_${discov_period}_${msg_per_fault}_2"

echo ""
echo "----- LIMO 2 ended waiting $WAIT_TIME seconds.. -----"
echo ""

sleep $WAIT_TIME


echo "---------------------- ENDED -------------------------"
echo "--------------- FAULT $changes NODES -----------------"


echo "------------------------------------------------------"
echo "----------------- ENDED EXTRAS ---------------------"
