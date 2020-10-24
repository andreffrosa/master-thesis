#!/bin/bash

MIN_ARGS=1
if [ "$#" -lt $(($MIN_ARGS)) ]
then
   echo "FATAL ERROR"
   exit
fi

PROJ_DIR="../CommunicationPrimitives/"

grep --include=\*.{c,h} --color=always -rnw $PROJ_DIR -e $1
