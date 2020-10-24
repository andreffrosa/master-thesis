#!/bin/bash

PROJ_DIR=".."

cd $PROJ_DIR
echo $(pwd) 
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 . && make
