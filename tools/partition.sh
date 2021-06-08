#!/bin/bash
for i in $(seq 1 24); do 
  if [ $i -lt 10 ];
  then
	cat $1.txt | grep raspi-0$i > ${1}_raspi-0$i.txt
  else 
	cat $1.txt | grep raspi-$i > ${1}_raspi-$i.txt
  fi
done

