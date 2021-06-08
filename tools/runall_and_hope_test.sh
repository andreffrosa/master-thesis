#!/bin/bash

YGGHOME=/home/yggdrasil/

MULTITEST=/home/yggdrasil/bin/aggMultiTest
FLOWTEST=/home/yggdrasil/bin/aggFlowTest
GAPTEST=/home/yggdrasil/bin/aggGapTest
GAPBCASTTEST=/home/yggdrasil/bin/aggGapBcastTest
LIMOTEST=/home/yggdrasil/bin/aggLimoTest

SRDS18=/home/yggdrasil/srds18/

EXP_TIME=20
WAIT_TIME=2

RASPIS=24

run_command() {
  $1 | grep "raspi" | wc -l
}

check_command() {
  if [ $1 = $RASPIS ];
  then
    echo "OK"
  else
    echo "NOT OK"
  fi
}

#begin nofaults test
echo "------------------------------------------------------"
echo "----------------- BEGIN NOFAULTS ---------------------"

discov_period=1
msg_per_fault=10

for i in $(seq 1 5); do
  echo "multi $i starting.."
  run_command 'cmd/cmdexecuteexperience 127.0.0.1 5000 "$MULTITEST $discov_period $msg_per_fault"'
  check_command $?

  sleep $EXP_TIME

  run_command 'cmd/cmdterminateexperience 127.0.0.1 5000 "${SRDS18}nofaults_MultiTest_${discov_period}_${msg_per_fault}_$i"'  check_command $?

  echo "multi $i ended waiting $WAIT_TIME.."

  sleep $WAIT_TIME

  echo "flow $i starting.."

  run_command 'cmd/cmdexecuteexperience 127.0.0.1 5000 "$FLOWTEST $discov_period $msg_per_fault"'
  check_command $?

  sleep $EXP_TIME

  run_command 'cmd/cmdterminateexperience 127.0.0.1 5000 "${SRDS18}nofaults_FlowTest_${discov_period}_${msg_per_fault}_$i"'
  check_command $?

  echo "flow $i ended waiting $WAIT_TIME.."

  sleep $WAIT_TIME

  echo "limo $i starting.."

  run_command 'cmd/cmdexecuteexperience 127.0.0.1 5000 "$LIMOTEST $discov_period $msg_per_fault"'
  check_command $?

  sleep $EXP_TIME

  run_command 'cmd/cmdterminateexperience 127.0.0.1 5000 "${SRDS18}nofaults_LimoTest_${discov_period}_${msg_per_fault}_$i"'
  check_command $?

  echo "limo $i ended waiting $WAIT_TIME.."

  sleep $WAIT_TIME

  echo "gap $i starting.."

  runControlProcess 'cmd/cmdexecuteexperience 127.0.0.1 5000 "$GAPTEST $discov_period $msg_per_fault"'
  check_command $?

  sleep $EXP_TIME

  run_command 'cmd/cmdterminateexperience 127.0.0.1 5000 "${SRDS18}nofaults_GapTest_${discov_period}_${msg_per_fault}_$i"'
  check_command $?

  echo "gap $i ended waiting $WAIT_TIME.."

  sleep $WAIT_TIME

  echo "gapbcast $i starting.."

  run_command 'cmd/cmdexecuteexperience 127.0.0.1 5000 "$GAPBCASTTEST $discov_period $msg_per_fault"'
  check_command $?

  sleep $EXP_TIME

  run_command 'cmd/cmdterminateexperience 127.0.0.1 5000 "${SRDS18}nofaults_GapBcastTest_${discov_period}_${msg_per_fault}_$i"'
  check_command $?

  echo "gapBcast $i ended waiting $WAIT_TIME.."

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
