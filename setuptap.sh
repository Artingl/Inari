#!/bin/sh
sudo ip link add name br0 type bridge
sudo ip link set br0 up
sudo ip link set enp4s0 master br0
sudo ip addr flush dev enp4s0
sudo dhcpcd br0 || sudo nmcli device connect br0
sudo ip tuntap add name tap0 mode tap user $(whoami)
sudo ip link set tap0 master br0
sudo ip link set tap0 up
# sudo ip tuntap add dev tap0 mode tap user $(whoami)
# sudo ip link set tap0 promisc on
# sudo ip link set tap0 up
# sudo sysctl -w net.ipv4.conf.tap0.rp_filter=0
# sudo sysctl -w net.ipv4.conf.all.rp_filter=0
# sudo ip addr add 192.168.99.10/24 dev tap0
