#!/bin/bash

MIN_ARGS=1
if [ "$#" -lt $(($MIN_ARGS)) ]
then
   echo "FATAL ERROR"
   exit
fi

RADIOS=${1:-$((3))}

# Remove previous wireless interfaces, if any
sudo modprobe -r mac80211_hwsim

# Create new virtual wireless interfaces
sudo modprobe mac80211_hwsim radios=$RADIOS channels=1 support_p2p_device=1

# Read the MAC addresses of the pis
db="$(cat ../topologies/line/macAddrDB.txt)"

# Parse file into array of lines
SAVEIFS=$IFS   # Save current IFS
IFS=$'\n'      # Change IFS to new line
db=($db)       # split to array

# For each line, until the end of the file or the num of radios is reached
# set the MAC address of the interface
for (( i=0; i<${#db[@]} && $i < $RADIOS; i++ ))
do
    IFS=' - '
    addrs=(${db[$i]})

    wlan=" wlan"$i

    PI=$(($i+1))

    echo "pi "$PI" :"

    STATUS=$((0))

    while [ $(($STATUS)) -eq $((0)) ]
    do
        echo -e "\tsetting interface ..."

        $(sudo nmcli dev set $wlan managed no)
        $(sudo ip link set dev $wlan down)
        $(sudo ip link set $wlan address ${addrs[1]})
        $(sudo iw $wlan set type ibss)
        $(sudo ip link set dev $wlan up)

        sleep .5

        STATUS=$(sudo ip a show $wlan | grep ",UP" | wc -l)

        if [ $(($STATUS)) -eq $((0)) ]
        then
            sleep .5
            STATUS=$(sudo ip a show $wlan | grep ",UP" | wc -l)
        fi

#        if [ $(($STATUS)) -eq $((1)) ]
#        then
#            STATUS2=$((1))
#            while [ $(($STATUS2)) -eq $((1)) ]
#            do
#                MSG=$(sudo iw $wlan ibss join "pis" 2462) # atrofia os pis a ligarem-se
#
#                echo $MSG
#
#                echo -e "\tjoining network ..."
#
#                STATUS3=$((0))
#                while [ $(($STATUS3)) -eq $((0)) ]
#                do
#                    sleep .25
#                    STATUS3=$(sudo iw $wlan info | grep ssid | wc -l)
#                done
#                sleep 1
#                #STATUS2=$(sudo ip a show $wlan | grep "CARRIER" | wc -l)
#                echo $(sudo iw $wlan info)
#                STATUS2=$((0))
#            done
#        fi
    done

    echo -e "\tpi "$PI" connected!\n"
    #echo "status: "$STATUS
    #$(sudo iw dev $wlan ibss leave)

    #$(sudo iw $wlan ibss join "pis" 2462) # atrofia os pis a ligarem-se

    #echo $(iw wlan2 info | grep channel)
    # TODO: catch errors and try again
done


IFS=$SAVEIFS   # Restore IFS

echo "DONE"
