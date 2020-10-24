#!/bin/bash

PROJ_DIR=".."

MIN_ARGS=3 #3

if [ "$#" -lt $(($MIN_ARGS)) ]
then
   echo "FATAL ERROR"
   exit
fi

#EXP="./experiments/"$3".conf"
#OVERLAY="./topologies/local_topologies/"$2"/"$1"/"

#APP="bcast_framework_test"
#APP="topology_discovery_test"
APP=$2

HOSTNAME="ERROR"
if [ $(($1)) -lt $((10)) ]
then
    HOSTNAME="raspi-0"$1
else
    HOSTNAME="raspi-"$1
fi

INTERFACE="wlan"$(($1-1))

#CMD="sudo gdb --args ./bin/"$APP" -f "$EXP" -o "$OVERLAY" -I "$INTERFACE" -H "$HOSTNAME" "$4" "$5

#CMD="sudo gdb --args ./bin/"$APP" -o "$OVERLAY" -I "$INTERFACE" -H "$HOSTNAME

NEW_ARGS=''
for i in "${@:$MIN_ARGS}"; do
    case "$i" in
        *\'*)
            i=`printf "%s" "$i" | sed "s/'/'\"'\"'/g"`
            ;;
        *) : ;;
    esac
    NEW_ARGS="$NEW_ARGS '$i'"
done

CMD="sudo gdb --args "$PROJ_DIR"/bin/"$APP" -I "$INTERFACE" -H "$HOSTNAME" "$NEW_ARGS

echo -e "\n"$CMD"\n"

eval $CMD
