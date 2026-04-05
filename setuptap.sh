#!/bin/sh
# sudo ip link add name br0 type bridge
# sudo ip link set br0 up
# sudo ip link set enp4s0 master br0
# sudo ip addr flush dev enp4s0
# sudo dhcpcd br0 || sudo nmcli device connect br0
# sudo ip tuntap add name tap0 mode tap user $(whoami)
# sudo ip link set tap0 master br0
# sudo ip link set tap0 up
# sudo ip link set br0 promisc on
# sudo ip link set enp4s0 promisc on
# sudo bridge link set dev enp4s0 hairpin on

# sudo ip tuntap add dev tap0 mode tap user $(whoami)
# sudo ip link set tap0 promisc on
# sudo ip link set tap0 up
# sudo sysctl -w net.ipv4.conf.tap0.rp_filter=0
# sudo sysctl -w net.ipv4.conf.all.rp_filter=0
# sudo ip addr add 192.168.99.10/24 dev tap0


sudo ip tuntap add dev tap0 mode tap user $(whoami)
sudo ip link set tap0 up
sudo ip addr add 192.168.53.1/24 dev tap0
sudo sysctl -w net.ipv4.ip_forward=1
sudo iptables -t nat -A POSTROUTING -o enp4s0 -j MASQUERADE
sudo iptables -A FORWARD -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
sudo iptables -A FORWARD -i tap0 -o enp4s0 -j ACCEPT


# sudo dnsmasq --interface=tap0 \
#              --bind-interfaces \
#              --dhcp-range=192.168.53.10,192.168.53.100,24h \
#              --dhcp-option=3,192.168.53.1 \
#              --no-daemon
