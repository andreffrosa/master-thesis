#!/bin/bash

YGGHOME=/home/yggdrasil/

MULTITEST=/home/yggdrasil/bin/aggMultiTest
FLOWTEST=/home/yggdrasil/bin/aggFlowTest
GAPTEST=/home/yggdrasil/bin/aggGapTest
GAPBCASTTEST=/home/yggdrasil/bin/aggGapBcastTest
LIMOTEST=/home/yggdrasil/bin/aggLimoTest

SRDS18=/home/yggdrasil/srds18/

EXP_TIME=30
WAIT_TIME=5

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

#begin nofaults test
echo "------------------------------------------------------"
echo "----------------- BEGIN NOFAULTS ---------------------"

discov_period=1
msg_per_fault=10

for i in $(seq 1 2); do
  echo "multi $i starting.."
  
  echo setting up tree...
  setup_tree

  sleep $WAIT_TIME

  check_tree
  echo "Check Support tree: "$(check_command $?)

  disable_announces
  echo "Disable Announces: "$(check_command $?)

  start_experience "$MULTITEST $discov_period $msg_per_fault"

  sleep $EXP_TIME

  stop_experience "${SRDS18}nofaults_MultiTest_${discov_period}_${msg_per_fault}_$i"

  enable_announces
  echo "Enabling Announces: "$(check_command $?)

  sleep $WAIT_TIME

  echo "multi $i ended waiting $WAIT_TIME.."

  sleep $WAIT_TIME

done

echo "------------------------------------------------------"
echo "----------------- ENDED NOFAULTS ---------------------"


#begin nofaults with dynamic values
#1 node change values

#3 node change values

#6 node change values

#12 nodes change values

#24 nodes change values

#begin node faults test
#1 node dies

#3 node dies

#6 node dies

#12 node dies

#begin link fault test
#1 link fault

#3 link fault

#6 link faults

#12 link fault
