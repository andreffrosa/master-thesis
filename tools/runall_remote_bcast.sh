#!/bin/bash

YGGHOME=/home/yggdrasil/

BCAST=/home/yggdrasil/bin/bcast_generic_test

PARAMS="2000 50 0 0"
FLOODING="$BCAST flooding $PARAMS"
BCAST_V0="$BCAST v0 $PARAMS"
BCAST_V1="$BCAST v1 $PARAMS"
BCAST_V2="$BCAST v2 $PARAMS"
BCAST_V4="$BCAST v4 $PARAMS"

SRDS18=/home/yggdrasil/bcast/

EXP_TIME=600
WAIT_TIME=5
WAIT_TIME_TO_RUN=30

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

n_runs=1
start_on=1
end_on=1

#echo "Discovery period: $discov_period"
#echo "Messages Per Fault: $msg_per_fault"
echo "Experience Time: $EXP_TIME"
echo "Number of runs: $n_runs"

echo setting up tree...
setup_tree

sleep $WAIT_TIME

check_tree
echo "Check Support tree: "$(check_command $?)

sleep $WAIT_TIME

echo "Disabling annouces.."
disable_announces
echo "Disable Announces: "$(check_command $?)

if (($start_on < 2)); then

  echo "------------------------------------------------------"
  echo "----------------- BEGIN NOFAULTS ---------------------"

  for i in $(seq 3 $n_runs); do
    echo ""
    echo "----------------- FLOODING $i starting.. ----------------"
    echo ""
    echo "setting up tree..."
    setup_tree

    sleep $WAIT_TIME

    check_tree
    echo "Check Support tree: "$(check_command $?)

    sleep $WAIT_TIME

    start_experience "$FLOODING"

    sleep $EXP_TIME

    stop_experience "${SRDS18}flooding_$i"

    echo ""
    echo "---- FLOODING $i ended waiting $WAIT_TIME_TO_RUN seconds.. -----"
    echo ""

    sleep $WAIT_TIME_TO_RUN

    echo ""
    echo "---------------- BCAST_V0 $i starting.. ------------------"
    echo ""
    setup_tree

    sleep $WAIT_TIME

    check_tree
    echo "Check Support tree: "$(check_command $?)

    sleep $WAIT_TIME

    start_experience "$BCAST_V0"

    sleep $EXP_TIME

    stop_experience "${SRDS18}bcast_v0_$i"

    echo ""
    echo "----- BCAST_V0 $i ended waiting $WAIT_TIME_TO_RUN seconds.. -----"
    echo ""

    sleep $WAIT_TIME_TO_RUN

    echo ""
    echo "----------------- BCAST_V1 $i starting.. -----------------"
    echo ""

    setup_tree

    sleep $WAIT_TIME

    check_tree
    echo "Check Support tree: "$(check_command $?)

    sleep $WAIT_TIME

    start_experience "$BCAST_V1"

    sleep $EXP_TIME

    stop_experience "${SRDS18}bcast_v1_$i"

    echo ""
    echo "----- BCAST_V1 $i ended waiting $WAIT_TIME_TO_RUN seconds.. -----"
    echo ""

    sleep $WAIT_TIME_TO_RUN

    echo ""
    echo "----------------- BCAST_V2 $i starting.. ------------------"
    echo ""

    setup_tree

    sleep $WAIT_TIME

    check_tree
    echo "Check Support tree: "$(check_command $?)

    sleep $WAIT_TIME

    start_experience "$BCAST_V2"

    sleep $EXP_TIME

    stop_experience "${SRDS18}bcast_v2_$i"

    echo ""
    echo "----- BCAST_V2 $i ended waiting $WAIT_TIME_TO_RUN seconds.. ------"
    echo ""

    sleep $WAIT_TIME_TO_RUN

    echo ""
    echo "-------------- BCAST_V4 $i starting.. ----------------"
    echo ""
    setup_tree

    sleep $WAIT_TIME

    check_tree
    echo "Check Support tree: "$(check_command $?)

    sleep $WAIT_TIME

    start_experience "$BCAST_V4"

    sleep $EXP_TIME

    stop_experience "${SRDS18}bcast_v4_$i"

    echo ""
    echo "--- BCAST_V4 $i ended waiting $WAIT_TIME_TO_RUN seconds.. ---"
    echo ""
    sleep $WAIT_TIME_TO_RUN
  done

  echo "------------------------------------------------------"
  echo "----------------- ENDED NOFAULTS ---------------------"

  sleep $WAIT_TIME
else
  echo "------------------------------------------------------"
  echo "--------------- SKIPPING NOFAULTS --------------------"
fi

echo ""
echo "------------------------------------------------------"
echo "--------------------- EXTRAS -------------------------"

sleep $WAIT_TIME_TO_RUN

echo ""
echo "-------------- BCAST_V4 2 starting.. ----------------"
echo ""
setup_tree

sleep $WAIT_TIME

check_tree
echo "Check Support tree: "$(check_command $?)

sleep $WAIT_TIME

start_experience "$BCAST_V4"

sleep $EXP_TIME

stop_experience "${SRDS18}bcast_v4_2"

echo ""
echo "--- BCAST_V4 2 ended waiting $WAIT_TIME_TO_RUN seconds.. ---"
echo ""
