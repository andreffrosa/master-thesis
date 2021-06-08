#!/bin/bash

echo $(date) > /home/yggdrasil/arp-output.txt
sleep 60
echo $(date) >> /home/yggdrasil/arp-output.txt
sudo arp -i wlan0 -f /etc/ethers 2>> /home/yggdrasil/arp-output.txt
tools/execControlProcess.sh $1 &
