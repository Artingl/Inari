#!/bin/sh
sudo ip tuntap add dev tap0 mode tap user $(whoami)
sudo ip link set tap0 promisc on
sudo ip link set tap0 up
sudo sysctl -w net.ipv4.conf.tap0.rp_filter=0
sudo sysctl -w net.ipv4.conf.all.rp_filter=0
sudo ip addr add 192.168.99.10/24 dev tap0
