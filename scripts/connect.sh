#!/bin/bash

MIN_ARGS=1
if [ "$#" -lt $(($MIN_ARGS)) ]
then
   echo "FATAL ERROR"
   exit
fi

wlan="wlan"$(($1-1))
PI=$1

HOSTNAME="raspi-0"$PI
INTERFACE=$wlan #"wlan"$(($PI-1))

echo "Connecting virtual node $PI (HOSTNAME=$HOSTNAME INTERFACE=$INTERFACE) ..."

sudo ../bin/connect_test -i $INTERFACE -h $HOSTNAME > /dev/null 2>&1
RES=$?
while [ "$RES" -ne "0" ]; do
    sudo ../bin/connect_test -i $INTERFACE -h $HOSTNAME > /dev/null 2>&1
    RES=$?
    sleep 1
done
