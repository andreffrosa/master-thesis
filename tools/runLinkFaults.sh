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

EXP_TIME=$(($EXP_TIME/2))

discov_period=1
msg_per_fault=10

n_runs=3
start_on=2
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

echo "------------------------------------------------------"
echo "----------------- START LINK FAULTS ------------------"

echo ""

#begin link fault test
#1 link fault
#3 link fault
#6 link faults
#12 link fault
tokill=(2 7 23 22 18 17 4 1 7 9 19 16 21 20 9 14 24 15 2 6 4 6 3 4)

if (($start_on < 2)); then
  for changes in 1 12; do

    echo "------------------------------------------------------"
    echo "--------------- FAULT $changes LINKS -----------------"

    gap_root="raspi-01"

    for i in $(seq 1 $n_runs); do
      pis1=""
      for pi in ${tokill[@]:0:$(($changes*2))}; do
        pis1+="$pi "
      done

      pis=${pis1:0:$((${#pis1}-1))}
      echo "Pis to kill in this run: $pis"
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

      change_link "$pis"
      echo "Killed Pis: $pis"
      ENDTIME=$(date +%s)
      elapsed=$(($ENDTIME - $STARTTIME))

      sleep $((($EXP_TIME) - $elapsed))

      stop_experience "${SRDS18}link_faults_${changes}_aggMultiTest_${discov_period}_${msg_per_fault}_$i"

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

      change_link "$pis"
      echo "Killed Pis: $pis"
      ENDTIME=$(date +%s)
      elapsed=$(($ENDTIME - $STARTTIME))

      sleep $((($EXP_TIME) - $elapsed))

      stop_experience "${SRDS18}link_faults_${changes}_aggFlowTest_${discov_period}_${msg_per_fault}_$i"

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

      change_link "$pis"
      echo "Killed Pis: $pis"
      ENDTIME=$(date +%s)
      elapsed=$(($ENDTIME - $STARTTIME))

      sleep $((($EXP_TIME) - $elapsed))

      stop_experience "${SRDS18}link_faults_${changes}_aggLimoTest_${discov_period}_${msg_per_fault}_$i"

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

      start_experience "$GAPTEST $gap_root $discov_period $msg_per_fault"
      echo "root is $gap_root"

      sleep $EXP_TIME

      STARTTIME=$(date +%s)

      change_link "$pis"
      echo "Killed Pis: $pis"
      ENDTIME=$(date +%s)
      elapsed=$(($ENDTIME - $STARTTIME))

      sleep $((($EXP_TIME) - $elapsed))

      stop_experience "${SRDS18}link_faults_${changes}_aggGapTest_root_${gap_root}_${discov_period}_${msg_per_fault}_$i"

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

      change_link "$pis"
      echo "Killed Pis: $pis"
      ENDTIME=$(date +%s)
      elapsed=$(($ENDTIME - $STARTTIME))

      sleep $((($EXP_TIME) - $elapsed))

      stop_experience "${SRDS18}link_faults_${changes}_aggGapBcastTest_root_${gap_root}_${discov_period}_${msg_per_fault}_$i"

      echo ""
      echo "--- GAPBCAST $i ended waiting $WAIT_TIME seconds.. ---"
      echo ""

      sleep $WAIT_TIME
    done
    echo "---------------------- ENDED -------------------------"
    echo "--------------- FAULT $changes LINKS -----------------"
  done

fi

echo ""
echo "------------------------EXTRAS-------------------------"
echo ""

changes=1

echo "------------------------------------------------------"
echo "--------------- FAULT $changes LINKS -----------------"

gap_root="raspi-01"


pis1=""
for pi in ${tokill[@]:0:$(($changes*2))}; do
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

change_link "$pis"
echo "Killed Pis: $pis"
ENDTIME=$(date +%s)
elapsed=$(($ENDTIME - $STARTTIME))

sleep $((($EXP_TIME) - $elapsed))

stop_experience "${SRDS18}link_faults_${changes}_aggLimoTest_${discov_period}_${msg_per_fault}_2"

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

change_link "$pis"
echo "Killed Pis: $pis"
ENDTIME=$(date +%s)
elapsed=$(($ENDTIME - $STARTTIME))

sleep $((($EXP_TIME) - $elapsed))

stop_experience "${SRDS18}link_faults_${changes}_aggLimoTest_${discov_period}_${msg_per_fault}_3"

echo ""
echo "----- LIMO 3 ended waiting $WAIT_TIME seconds.. -----"
echo ""

sleep $WAIT_TIME

echo "---------------------- ENDED -------------------------"
echo "--------------- FAULT $changes LINKS -----------------"

echo ""
changes=12
echo ""

echo "------------------------------------------------------"
echo "--------------- FAULT $changes LINKS -----------------"

gap_root="raspi-01"

pis1=""
for pi in ${tokill[@]:0:$(($changes*2))}; do
  pis1+="$pi "
done

pis=${pis1:0:$((${#pis1}-1))}
echo "Pis to kill in this run: $pis"

echo ""
echo "----------------- GAP 3 starting.. ------------------"
echo ""

setup_tree

sleep $WAIT_TIME

check_tree
echo "Check Support tree: "$(check_command $?)

sleep $WAIT_TIME

start_experience "$GAPTEST $gap_root $discov_period $msg_per_fault"
echo "root is $gap_root"

sleep $EXP_TIME

STARTTIME=$(date +%s)

change_link "$pis"
echo "Killed Pis: $pis"
ENDTIME=$(date +%s)
elapsed=$(($ENDTIME - $STARTTIME))

sleep $((($EXP_TIME) - $elapsed))

stop_experience "${SRDS18}link_faults_${changes}_aggGapTest_root_${gap_root}_${discov_period}_${msg_per_fault}_3"

echo ""
echo "----- GAP 3 ended waiting $WAIT_TIME seconds.. ------"
echo ""

sleep $WAIT_TIME


echo "---------------------- ENDED -------------------------"
echo "--------------- FAULT $changes LINKS -----------------"
