#!/bin/bash

MIN_ARGS=1
if [ "$#" -lt $(($MIN_ARGS)) ]
then
   echo "usage: ./kill_test.sh <str>"
   exit
fi

sudo kill -9 $(ps ax | grep "$1" | grep -v "grep" | awk '{print $1}' ) > /dev/null 2>&1
ps ax | grep "$EXE" | grep -v "grep" | awk '{print $1}'

