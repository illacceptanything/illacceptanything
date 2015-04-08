#!/bin/bash
# Install and load the netcat kernel module for your listening pleasure.
#
#

sudo apt-get install build-essential vorbis-tools linux-headers-$(uname -r)

cd /tmp && git clone https://github.com/usrbinnc/netcat-cpi-kernel-module.git
cd /tmp/netcat-cpi-kernel-module

make
sudo insmod netcat.ko

dmesg | tail -n 10

echo "// Starting album playback..."

ogg123 - < /dev/netcat 
