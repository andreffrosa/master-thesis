
sudo ip link set dev $1 down
sudo ip link set $1 address $2
sudo ip link set dev $1 up
