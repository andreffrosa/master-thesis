#!/bin/bash

if [ "$#" -lt $((1)) ]
then
   echo "FATAL ERROR"
   exit
fi

LOG="../debug/valgrind-"$(date +"%T-%d-%m-%y")".valgrind"

#BIN=$(which $1)

NEW_ARGS=''
for i in "$@"; do
    case "$i" in
        *\'*)
            i=`printf "%s" "$i" | sed "s/'/'\"'\"'/g"`
            ;;
        *) : ;;
    esac
    NEW_ARGS="$NEW_ARGS '$i'"
done

CMD="G_SLICE=always-malloc G_DEBUG=gc-friendly sudo valgrind -v --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes --num-callers=40 --log-file="$LOG" "$NEW_ARGS

echo -e "\n"$CMD"\n"

eval $CMD
