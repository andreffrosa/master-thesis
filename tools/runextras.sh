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
echo "----------------- BEGIN EXTRAS ---------------------"

gap_root="raspi-01"

echo ""
echo "------------------ Change 1 1 -------------------------"

arg="15 96"


echo ""
echo "------------------ Change 1 2 -------------------------"

arg="13 191"


echo ""
echo "-------------- GAPBCAST starting.. ----------------"
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


stop_experience "${SRDS18}nofaults_dynamic_1_aggGapBcastTest_root_${gap_root}_${discov_period}_${msg_per_fault}_2"

echo ""
echo "--- GAPBCAST  ended waiting $WAIT_TIME seconds.. ---"
echo ""

sleep $WAIT_TIME

echo ""
echo "------------------ Change 1 3 -------------------------"

arg="5 45"


echo ""
echo "-------------- GAPBCAST starting.. ----------------"
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


stop_experience "${SRDS18}nofaults_dynamic_1_aggGapBcastTest_root_${gap_root}_${discov_period}_${msg_per_fault}_3"

echo ""
echo "--- GAPBCAST  ended waiting $WAIT_TIME seconds.. ---"
echo ""

sleep $WAIT_TIME

echo ""
echo "------------------ Change 12 1 -------------------------"

arg="3 54 18 130 10 34 19 111 1 126 24 88 8 88 2 47 22 90 7 80 11 200 23 130"

echo ""
echo "---------------- FLOW  starting.. ------------------"
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

stop_experience "${SRDS18}nofaults_dynamic_12_aggFlowTest_${discov_period}_${msg_per_fault}_1"

echo ""
echo "----- FLOW ended waiting $WAIT_TIME seconds.. -----"
echo ""

sleep $WAIT_TIME

echo ""
echo "-------------- GAPBCAST starting.. ----------------"
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


stop_experience "${SRDS18}nofaults_dynamic_12_aggGapBcastTest_root_${gap_root}_${discov_period}_${msg_per_fault}_1"

echo ""
echo "--- GAPBCAST  ended waiting $WAIT_TIME seconds.. ---"
echo ""

sleep $WAIT_TIME

echo ""
echo "------------------ Change 12 2 -------------------------"

arg="6 119 24 13 8 9 1 29 13 44 22 143 18 18 15 16 2 135 3 72 16 128 11 146"

echo ""
echo "---------------- FLOW  starting.. ------------------"
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

stop_experience "${SRDS18}nofaults_dynamic_12_aggFlowTest_${discov_period}_${msg_per_fault}_2"

echo ""
echo "----- FLOW ended waiting $WAIT_TIME seconds.. -----"
echo ""

sleep $WAIT_TIME

echo ""
echo "------------------ Change 12 3 -------------------------"

arg="8 27 19 89 1 32 18 76 24 79 9 65 12 151 4 47 3 20 10 164 13 104 7 179"

echo ""
echo "---------------- FLOW  starting.. ------------------"
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

stop_experience "${SRDS18}nofaults_dynamic_12_aggFlowTest_${discov_period}_${msg_per_fault}_3"

echo ""
echo "----- FLOW ended waiting $WAIT_TIME seconds.. -----"
echo ""

sleep $WAIT_TIME

echo ""
echo "-------------- GAPBCAST starting.. ----------------"
echo ""
setup_tree

sleep $WAIT_TIME

check_tree
echo "Check Support tree: "$(check_command $?)

start_experience "$GAPBCASTTEST $gap_root $discov_period $msg_per_fault"
echo "root is $gap_root"
sleep $EXP_TIME

STARTTIME=$(date +%s)

change_value "$arg"

ENDTIME=$(date +%s)
elapsed=$(($ENDTIME - $STARTTIME))

sleep $((($EXP_TIME) - $elapsed))

sleep $WAIT_TIME

stop_experience "${SRDS18}nofaults_dynamic_12_aggGapBcastTest_root_${gap_root}_${discov_period}_${msg_per_fault}_3"

echo ""
echo "--- GAPBCAST  ended waiting $WAIT_TIME seconds.. ---"
echo ""

sleep $WAIT_TIME

echo ""
echo "---------------- LIMO  starting.. ------------------"
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

stop_experience "${SRDS18}nofaults_dynamic_12_aggLimoTest_${discov_period}_${msg_per_fault}_3"

echo ""
echo "----- LIMO ended waiting $WAIT_TIME seconds.. -----"
echo ""

sleep $WAIT_TIME

echo ""
echo "------------------ Change 24 1 -------------------------"

arg="18 121 11 85 24 117 15 101 2 54 1 145 9 91 22 55 10 199 21 15 23 85 12 49 7 106 13 59 16 16 8 147 6 23 14 109 19 115 20 90 4 186 17 83 5 123 3 43"

echo ""
echo "---------------- FLOW  starting.. ------------------"
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

stop_experience "${SRDS18}nofaults_dynamic_24_aggFlowTest_${discov_period}_${msg_per_fault}_1"

echo ""
echo "----- FLOW ended waiting $WAIT_TIME seconds.. -----"
echo ""

sleep $WAIT_TIME

echo ""
echo "-------------- GAP starting.. ----------------"
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

change_value "$arg"

ENDTIME=$(date +%s)
elapsed=$(($ENDTIME - $STARTTIME))

sleep $((($EXP_TIME) - $elapsed))


stop_experience "${SRDS18}nofaults_dynamic_24_aggGapTest_root_${gap_root}_${discov_period}_${msg_per_fault}_1"

echo ""
echo "--- GAP  ended waiting $WAIT_TIME seconds.. ---"
echo ""

sleep $WAIT_TIME


echo ""
echo "------------------ Change 24 2 -------------------------"

arg="17 27 1 39 7 71 20 23 6 58 23 48 18 178 4 194 12 104 8 130 14 149 19 47 21 190 2 133 10 10 15 42 5 21 11 168 3 117 22 69 16 145 24 55 9 87 13 83"

echo ""
echo "-------------- GAPBCAST starting.. ----------------"
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


stop_experience "${SRDS18}nofaults_dynamic_24_aggGapBcastTest_root_${gap_root}_${discov_period}_${msg_per_fault}_2"

echo ""
echo "--- GAPBCAST  ended waiting $WAIT_TIME seconds.. ---"
echo ""

sleep $WAIT_TIME

echo ""
echo "---------------- MULTI  starting.. ------------------"
echo ""
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

stop_experience "${SRDS18}nofaults_dynamic_24_aggMultiTest_${discov_period}_${msg_per_fault}_2"

echo ""
echo "----- MULTI ended waiting $WAIT_TIME seconds.. -----"
echo ""

sleep $WAIT_TIME

echo ""
echo "------------------ Change 24 3 -------------------------"

arg="1 123 12 36 8 189 9 170 4 12 20 82 3 2 2 45 6 45 17 13 14 32 22 99 18 116 23 35 10 138 7 90 5 134 11 73 19 105 13 163 15 84 16 131 24 141 21 190"

echo ""
echo "---------------- FLOW  starting.. ------------------"
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

stop_experience "${SRDS18}nofaults_dynamic_24_aggFlowTest_${discov_period}_${msg_per_fault}_3"

echo ""
echo "----- FLOW ended waiting $WAIT_TIME seconds.. -----"
echo ""

sleep $WAIT_TIME

echo ""
echo "---------------- LIMO  starting.. ------------------"
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

stop_experience "${SRDS18}nofaults_dynamic_24_aggLimoTest_${discov_period}_${msg_per_fault}_3"

echo ""
echo "----- LIMO ended waiting $WAIT_TIME seconds.. -----"
echo ""

sleep $WAIT_TIME

echo ""
echo "-------------- GAP starting.. ----------------"
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

change_value "$arg"

ENDTIME=$(date +%s)
elapsed=$(($ENDTIME - $STARTTIME))

sleep $((($EXP_TIME) - $elapsed))

stop_experience "${SRDS18}nofaults_dynamic_24_aggGapTest_root_${gap_root}_${discov_period}_${msg_per_fault}_3"

echo ""
echo "--- GAP  ended waiting $WAIT_TIME seconds.. ---"
echo ""

sleep $WAIT_TIME

echo "------------------------------------------------------"
echo "----------------- ENDED EXTRAS ---------------------"

sleep $WAIT_TIME
