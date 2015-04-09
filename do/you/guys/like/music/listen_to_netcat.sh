#!/bin/bash
# Install and load the netcat kernel module for your listening pleasure.

set -e

if [ "$(id -u)" != "0" ]; then
  echo "This script must be run as root" 1>&2
  exit 1
fi

sudo apt-get install build-essential vorbis-tools linux-headers-$(uname -r)

cd /tmp && git clone https://github.com/usrbinnc/netcat-cpi-kernel-module.git
cd /tmp/netcat-cpi-kernel-module

make
sudo insmod netcat.ko

dmesg | tail -n 10

echo "// Starting album playback..."

ogg123 - < /dev/netcat 
